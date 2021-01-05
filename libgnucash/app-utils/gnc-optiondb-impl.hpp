/********************************************************************\
 * gnc-optiondb.hpp -- Collection of GncOption objects              *
 * Copyright (C) 2019 John Ralls <jralls@ceridwen.us>               *
 *                                                                  *
 * This program is free software; you can redistribute it and/or    *
 * modify it under the terms of the GNU General Public License as   *
 * published by the Free Software Foundation; either version 2 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU General Public License for more details.                     *
 *                                                                  *
 * You should have received a copy of the GNU General Public License*
 * along with this program; if not, contact:                        *
 *                                                                  *
 * Free Software Foundation           Voice:  +1-617-542-5942       *
 * 51 Franklin Street, Fifth Floor    Fax:    +1-617-542-2652       *
 * Boston, MA  02110-1301,  USA       gnu@gnu.org                   *
 *                                                                  *
\********************************************************************/

#ifndef GNC_OPTIONDB_P_HPP_
#define GNC_OPTIONDB_P_HPP_

#include "gnc-option.hpp"
#include "gnc-option-impl.hpp"

#include <functional>
#include <exception>
#include <optional>
#include <iostream>
extern "C"
{
#include <config.h>
#include <qof.h>
#include <gncInvoice.h>
#include <gncOwner.h>
#include <gncTaxTable.h>
}

using GncOptionVec = std::vector<GncOption>;

class GncOptionSection
{
    std::string m_name;
    GncOptionVec m_options;
public:
    GncOptionSection(const char* name) : m_name{name}, m_options{} {}
    ~GncOptionSection() = default;

    void foreach_option(std::function<void(GncOption&)> func);
    void foreach_option(std::function<void(const GncOption&)> func) const;
    const std::string& get_name() const noexcept { return m_name; }
    size_t get_num_options() const noexcept { return m_options.size(); }
    void add_option(GncOption&& option);
    void remove_option(const char* name);
    const GncOption* find_option(const char* name) const;
};

using GncOptionSectionPtr = std::shared_ptr<GncOptionSection>;

class GncOptionDB
{
public:
    GncOptionDB();
    GncOptionDB(QofBook* book);
    ~GncOptionDB() = default;

/* The non-const version can't be redirected to the const one because the
 * function parameters are incompatible.
 */
    void foreach_section(std::function<void(GncOptionSectionPtr&)> func)
    {
        for (auto& section : m_sections)
            func(section);
    }
    void foreach_section(std::function<void(const GncOptionSectionPtr&)> func) const
    {
        for (auto& section : m_sections)
            func(section);
    }
    size_t num_sections() const noexcept { return m_sections.size(); }
    bool get_changed() const noexcept { return m_dirty; }
    void register_option(const char* section, GncOption&& option);
    void register_option(const char* section, GncOption* option);
    void unregister_option(const char* section, const char* name);
    void set_default_section(const char* section);
    const GncOptionSection* const get_default_section() const noexcept;
    std::string lookup_string_option(const char* section, const char* name);
    template <typename ValueType>
    bool set_option(const char* section, const char* name, ValueType value)
    {
        try
        {
            auto option{find_option(section, name)};
            if (!option)
                return false;
            option->set_value(value);
            return true;
        }
        catch(const std::invalid_argument& err)
        {
            printf("Set Failed: %s\n", err.what());
            return false;
        }
    }
//    void set_selectable(const char* section, const char* name);
    void make_internal(const char* section, const char* name);
    void commit() {};
    GncOptionSection* find_section(const std::string& sectname)
    {
        return const_cast<GncOptionSection*>(static_cast<const GncOptionDB&>(*this).find_section(sectname));
    }
    const GncOptionSection* find_section(const std::string& sectname) const;
    GncOption* find_option(const std::string& section, const char* name)
    {
        return const_cast<GncOption*>(static_cast<const GncOptionDB&>(*this).find_option(section, name));
    }
    const GncOption* find_option(const std::string& section, const char* name) const;
    std::ostream& save_to_scheme(std::ostream& oss,
                                 const char* options_prolog) const noexcept;
    std::istream& load_from_scheme(std::istream& iss) noexcept;
    std::ostream& save_to_key_value(std::ostream& oss) const noexcept;
    std::istream& load_from_key_value(std::istream& iss);
    void save_to_kvp(QofBook* book, bool clear_book) const noexcept;
    void load_from_kvp(QofBook* book) noexcept;
    std::ostream& save_option_scheme(std::ostream& oss,
                                     const char* option_prolog,
                                     const std::string& section,
                                     const std::string& name) const noexcept;
    std::istream& load_option_scheme(std::istream& iss);
    std::ostream& save_option_key_value(std::ostream& oss,
                                        const std::string& section,
                                        const std::string& name) const noexcept;
    std::istream& load_option_key_value(std::istream& iss);
private:
    GncOptionSection* m_default_section;
    std::vector<GncOptionSectionPtr> m_sections;
    bool m_dirty = false;

    std::function<GncOptionUIItem*()> m_get_ui_value;
    std::function<void(GncOptionUIItem*)> m_set_ui_value;
    static constexpr char const* const scheme_tags[]
    {
        "(let ((option (gnc:lookup-option ",
        "                                 ",
        ")))",
        "   ((lambda (o) (if o (gnc:option-set-value o ",
        "))) option))"
        };
};


#endif // GNC_OPTIONDB_P_HPP_
