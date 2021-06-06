#pragma once

// #include <leveldb/db.h>
#include <array>

#include <optional>

// #include <boost/algorithm/string.hpp>
// #include <tbb/concurrent_unordered_map.h>

struct IgnoreCaseLess
{
    using is_transparent = int;

    template< class T, class U>
    inline auto operator()(T&& lhs, U&& rhs) const
        -> decltype(std::forward<T>(lhs) < std::forward<U>(rhs))
    {
        return 1;
    }
};

class ResourceCacheEntryList : public fwRefCountable, public fx::IAttached<fx::Resource>
{
public:
    struct Entry
    {
        std::string resourceName;
        std::string basename;
        std::string remoteUrl;
        std::string referenceHash;
        size_t size;
        std::map<std::string, std::string> extData;

        inline Entry()
        {

        }

        inline Entry(const std::string& resourceName, const std::string& basename, const std::string& remoteUrl, const std::string& referenceHash, size_t size, const std::map<std::string, std::string>& extData = {})
            : resourceName(resourceName), basename(basename), remoteUrl(remoteUrl), referenceHash(referenceHash), size(size), extData(extData)
        {

        }
    };

private:
    fx::Resource* m_parentResource;

    std::map<std::string, Entry, IgnoreCaseLess> m_entries;

public:
    virtual void AttachToObject(fx::Resource* resource) {};

    inline const std::map<std::string, Entry, IgnoreCaseLess>& GetEntries()
    {
        return m_entries;
    }

    inline std::optional<std::reference_wrapper<const Entry>> GetEntry(std::string_view baseName)
    {
        auto it = m_entries.find(baseName);

        if (it == m_entries.end())
        {
            return {};
        }

        return it->second;
    }

    inline void AddEntry(const Entry& entry)
    {
        m_entries[entry.basename] = entry;
        m_entries[entry.basename].resourceName = m_parentResource->GetName();
    }
};

DECLARE_INSTANCE_TYPE(ResourceCacheEntryList);