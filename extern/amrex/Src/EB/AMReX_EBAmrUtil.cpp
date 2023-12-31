
#include <AMReX_EBAmrUtil.H>
#include <AMReX_EBFArrayBox.H>
#include <AMReX_EBCellFlag.H>
#include <AMReX_iMultiFab.H>

#ifdef AMREX_USE_OMP
#include <omp.h>
#endif

namespace amrex {

void
TagCutCells (TagBoxArray& tags, const MultiFab& state)
{
    const char   tagval = TagBox::SET;
//    const char clearval = TagBox::CLEAR;

    auto const& factory = dynamic_cast<EBFArrayBoxFactory const&>(state.Factory());

    if (factory.hasEBInfo()) {
        auto const& flags = factory.getMultiEBCellFlagFab();

#ifdef AMREX_USE_OMP
#pragma omp parallel if (Gpu::notInLaunchRegion())
#endif
        for (MFIter mfi(state, TilingIfNotGPU()); mfi.isValid(); ++mfi)
        {
            const Box& bx = mfi.tilebox();

            const auto& flag = flags[mfi];

            const FabType typ = flag.getType(bx);
            if (typ != FabType::regular && typ != FabType::covered)
            {
                Array4<char> const& tagarr = tags.array(mfi);
                Array4<EBCellFlag const> const& flagarr = flag.const_array();
                AMREX_HOST_DEVICE_FOR_3D (bx, i, j, k,
                {
                    if (flagarr(i,j,k).isSingleValued()) {
                        tagarr(i,j,k) = tagval;
                    }
                });
            }
        }
    } else {
        auto const* cutcell_mask = factory.getCutCellMask();
        if (cutcell_mask) {
            auto const& ta = tags.arrays();
            auto const& ma = cutcell_mask->const_arrays();
            ParallelFor(state, [=] AMREX_GPU_DEVICE (int bno, int i, int j, int k)
            {
                if (ma[bno](i,j,k)) {
                    ta[bno](i,j,k) = tagval;
                }
            });
            amrex::Gpu::streamSynchronize();
        }
    }
}


void
TagVolfrac (TagBoxArray& tags, const MultiFab& volfrac, Real tol)
{
    BL_PROFILE("amrex::TagVolfrac()");

//    const char clearval = TagBox::CLEAR;
    const char   tagval = TagBox::SET;

#ifdef AMREX_USE_OMP
#pragma omp parallel if (Gpu::notInLaunchRegion())
#endif
    for (MFIter mfi(volfrac, TilingIfNotGPU()); mfi.isValid(); ++mfi) {
        const Box& bx = mfi.tilebox();
        Array4<char> const& tagarr = tags.array(mfi);
        Array4<Real const> const& volarr = volfrac.const_array(mfi);
        AMREX_HOST_DEVICE_FOR_3D ( bx, i, j, k,
        {
            if (volarr(i,j,k) <= (1.-tol) && volarr(i,j,k) >= tol) {
                tagarr(i,j,k) = tagval;
            }
        });
    }
}

}
