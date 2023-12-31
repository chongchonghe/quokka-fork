
#include <AMReX_EBDataCollection.H>
#include <AMReX_MultiFab.H>
#include <AMReX_iMultiFab.H>
#include <AMReX_MultiCutFab.H>

#include <AMReX_EB2_Level.H>
#include <utility>

namespace amrex {

EBDataCollection::EBDataCollection (const EB2::Level& a_level,
                                    const Geometry& a_geom,
                                    const BoxArray& a_ba_in,
                                    const DistributionMapping& a_dm,
                                    Vector<int>  a_ngrow, EBSupport a_support)
    : m_ngrow(std::move(a_ngrow)),
      m_support(a_support),
      m_geom(a_geom)
{
    // The BoxArray argument may not be cell-centered BoxArray.
    const BoxArray& a_ba = amrex::convert(a_ba_in, IntVect::TheZeroVector());

    if (m_support >= EBSupport::basic)
    {
        m_cellflags = new FabArray<EBCellFlagFab>(a_ba, a_dm, 1, m_ngrow[0], MFInfo(),
                                                  DefaultFabFactory<EBCellFlagFab>());
        a_level.fillEBCellFlag(*m_cellflags, m_geom);
        m_levelset = new MultiFab(amrex::convert(a_ba,IntVect::TheUnitVector()), a_dm,
                                  1, m_ngrow[0], MFInfo(), FArrayBoxFactory());
        a_level.fillLevelSet(*m_levelset, m_geom);
    }

    if (m_support >= EBSupport::volume)
    {
        m_volfrac = new MultiFab(a_ba, a_dm, 1, m_ngrow[1], MFInfo(), FArrayBoxFactory());
        a_level.fillVolFrac(*m_volfrac, m_geom);

        m_centroid = new MultiCutFab(a_ba, a_dm, AMREX_SPACEDIM, m_ngrow[1], *m_cellflags);
        a_level.fillCentroid(*m_centroid, m_geom);
    }

    if (m_support == EBSupport::full)
    {
        const int ng = m_ngrow[2];

        m_bndrycent = new MultiCutFab(a_ba, a_dm, AMREX_SPACEDIM, ng, *m_cellflags);
        a_level.fillBndryCent(*m_bndrycent, m_geom);

        m_bndryarea = new MultiCutFab(a_ba, a_dm, 1, ng, *m_cellflags);
        a_level.fillBndryArea(*m_bndryarea, m_geom);

        m_bndrynorm = new MultiCutFab(a_ba, a_dm, AMREX_SPACEDIM, ng, *m_cellflags);
        a_level.fillBndryNorm(*m_bndrynorm, m_geom);

        for (int idim = 0; idim < AMREX_SPACEDIM; ++idim) {
            const BoxArray& faceba = amrex::convert(a_ba, IntVect::TheDimensionVector(idim));
            m_areafrac[idim] = new MultiCutFab(faceba, a_dm, 1, ng, *m_cellflags);
            m_facecent[idim] = new MultiCutFab(faceba, a_dm, AMREX_SPACEDIM-1, ng, *m_cellflags);
            IntVect edge_type{1}; edge_type[idim] = 0;
            m_edgecent[idim] = new MultiCutFab(amrex::convert(a_ba, edge_type), a_dm,
                                               1, ng, *m_cellflags);
        }

        a_level.fillAreaFrac(m_areafrac, m_geom);
        a_level.fillFaceCent(m_facecent, m_geom);
        a_level.fillEdgeCent(m_edgecent, m_geom);
    }

    if (! a_level.hasEBInfo()) {
        m_cutcellmask = new iMultiFab(a_ba, a_dm, 1, 0, MFInfo(),
                                      DefaultFabFactory<IArrayBox>());
        a_level.fillCutCellMask(*m_cutcellmask, m_geom);
    }
}

EBDataCollection::~EBDataCollection ()
{
    delete m_cellflags;
    delete m_levelset;
    delete m_volfrac;
    delete m_centroid;
    delete m_bndrycent;
    delete m_bndrynorm;
    delete m_bndryarea;
    for (int idim = 0; idim < AMREX_SPACEDIM; ++idim) {
        delete m_areafrac[idim];
        delete m_facecent[idim];
        delete m_edgecent[idim];
    }
    delete m_cutcellmask;
}

const FabArray<EBCellFlagFab>&
EBDataCollection::getMultiEBCellFlagFab () const
{
    AMREX_ASSERT(m_cellflags != nullptr);
    return *m_cellflags;
}

const MultiFab&
EBDataCollection::getLevelSet () const
{
    AMREX_ASSERT(m_levelset != nullptr);
    return *m_levelset;
}

const MultiFab&
EBDataCollection::getVolFrac () const
{
    AMREX_ASSERT(m_volfrac != nullptr);
    return *m_volfrac;
}

const MultiCutFab&
EBDataCollection::getCentroid () const
{
    AMREX_ASSERT(m_centroid != nullptr);
    return *m_centroid;
}

const MultiCutFab&
EBDataCollection::getBndryCent () const
{
    AMREX_ASSERT(m_bndrycent != nullptr);
    return *m_bndrycent;
}

const MultiCutFab&
EBDataCollection::getBndryArea () const
{
    AMREX_ASSERT(m_bndryarea != nullptr);
    return *m_bndryarea;
}

Array<const MultiCutFab*, AMREX_SPACEDIM>
EBDataCollection::getAreaFrac () const
{
    AMREX_ASSERT(m_areafrac[0] != nullptr);
    return {AMREX_D_DECL(m_areafrac[0], m_areafrac[1], m_areafrac[2])};
}

Array<const MultiCutFab*, AMREX_SPACEDIM>
EBDataCollection::getFaceCent () const
{
    AMREX_ASSERT(m_facecent[0] != nullptr);
    return {AMREX_D_DECL(m_facecent[0], m_facecent[1], m_facecent[2])};
}

Array<const MultiCutFab*, AMREX_SPACEDIM>
EBDataCollection::getEdgeCent () const
{
    AMREX_ASSERT(m_edgecent[0] != nullptr);
    return {AMREX_D_DECL(m_edgecent[0], m_edgecent[1], m_edgecent[2])};
}

const MultiCutFab&
EBDataCollection::getBndryNormal () const
{
    AMREX_ASSERT(m_bndrynorm != nullptr);
    return *m_bndrynorm;
}

const iMultiFab*
EBDataCollection::getCutCellMask () const
{
    return m_cutcellmask;
}

}
