#ifndef AMREX_LayoutData_H_
#define AMREX_LayoutData_H_
#include <AMReX_Config.H>

#include <AMReX_FabArrayBase.H>
#include <AMReX_MFIter.H>

namespace amrex
{
  //! a one-thingy-per-box distributed object
  template <class T>
  class LayoutData: public FabArrayBase
  {
  public:

    //!
    LayoutData() = default;

    LayoutData(const BoxArray            & a_grids,
               const DistributionMapping & a_dm)
      {
        define(a_grids, a_dm);
      }

    void define(const BoxArray           & a_grids,
                const DistributionMapping & a_dm)
      {
        FabArrayBase::define(a_grids, a_dm, 1, 0);
        m_need_to_clear_bd = true;
        addThisBD();
        m_data.resize(local_size());
      }

    ~LayoutData () { if (m_need_to_clear_bd) { clearThisBD(); } }

    LayoutData (const LayoutData& rhs) noexcept
        : FabArrayBase(rhs),
          m_data(rhs.m_data),
          m_need_to_clear_bd(rhs.m_need_to_clear_bd)
    {
        if (m_need_to_clear_bd) { addThisBD(); }
    }

    LayoutData (LayoutData&& rhs) noexcept
        : FabArrayBase(static_cast<FabArrayBase&&>(rhs)),
          m_data(std::move(rhs.m_data)),
          m_need_to_clear_bd(rhs.m_need_to_clear_bd)
    {
        rhs.m_need_to_clear_bd = false;
    }

    LayoutData& operator= (const LayoutData& rhs) noexcept
    {
        if (&rhs != this)
        {
            m_data.clear();
            if (m_need_to_clear_bd) { clearThisBD(); }

            FabArrayBase::operator=(rhs);
            m_data = rhs.m_data;
            m_need_to_clear_bd = rhs.m_need_to_clear_bd;

            if (m_need_to_clear_bd) { addThisBD(); }
        }
        return *this;
    }

    LayoutData& operator= (LayoutData&& rhs) noexcept
    {
        if (&rhs != this)
        {
            m_data.clear();
            if (m_need_to_clear_bd) { clearThisBD(); }

            FabArrayBase::operator=(static_cast<FabArrayBase&&>(rhs));
            m_data = std::move(rhs.m_data);
            m_need_to_clear_bd = rhs.m_need_to_clear_bd;

            rhs.m_need_to_clear_bd = false;
        }
        return *this;
    }

    T& operator[](const MFIter& a_mfi) noexcept
      {
        int local_index = a_mfi.LocalIndex();
        BL_ASSERT(local_index >= 0 && local_index < m_data.size() &&
                  DistributionMap() == a_mfi.DistributionMap());
        return m_data[local_index];
      }


    const T& operator[](const MFIter& a_mfi) const noexcept
      {
        int local_index = a_mfi.LocalIndex();
        BL_ASSERT(local_index >= 0 && local_index < m_data.size() &&
                  DistributionMap() == a_mfi.DistributionMap());
        return m_data[local_index];
      }

    T& operator[](int a_box_index) noexcept
      {
        int local_index = this->localindex(a_box_index);
        BL_ASSERT(local_index >= 0 && local_index < m_data.size());
        return m_data[local_index];
      }


    const T& operator[](int a_box_index) const noexcept
      {
        int local_index = this->localindex(a_box_index);
        BL_ASSERT(local_index >= 0 && local_index < m_data.size());
        return m_data[local_index];
      }

    const T* data () const noexcept { return m_data.data(); }
    T*       data ()       noexcept { return m_data.data(); }

  private:
      Vector<T> m_data;
      bool m_need_to_clear_bd = false;
  };
}
#endif
