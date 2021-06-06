#pragma once

#include "ObjectModel/core.h"

#include <optional>
#include "IteratorView.h"
#include "Registry.h"

namespace fx
{
	class Resource;

	class ResourceMetaDataComponent;

	class ResourceMetaDataLoader : public fwRefCountable
	{
	public:
		virtual /*boost::optional<std::string>*/ void LoadMetaData(ResourceMetaDataComponent* component, const std::string& resourcePath) = 0;
	};

	class ResourceMetaDataComponent : public fwRefCountable
	{
	public:
		Resource* m_resource;

		std::multimap<std::string, std::string> m_metaDataEntries;

		fwRefContainer<ResourceMetaDataLoader> m_metaDataLoader;

	public:
		ResourceMetaDataComponent(Resource* resourceRef);

		/*boost::optional<std::string>*/ void LoadMetaData(const std::string& resourcePath);

		std::optional<bool> IsManifestVersionBetween(const guid_t& lowerBound, const guid_t& upperBound);

		std::optional<bool> IsManifestVersionBetween(const std::string& lowerBound, const std::string& upperBound);

		inline Resource* GetResource()
		{
			return m_resource;
		}

		inline void SetMetaDataLoader(fwRefContainer<ResourceMetaDataLoader> loader)
		{
			m_metaDataLoader = loader;
		}

		inline auto GetEntries(const std::string& key)
		{
			return GetIteratorView(m_metaDataEntries.equal_range(key));
		}

		void GlobEntries(const std::string& key, const std::function<void(const std::string&)>& entryCallback);

		template<typename OutputIterator>
		inline void GlobEntriesIterator(const std::string& key, OutputIterator out)
		{
			GlobEntries(key, [&out](const std::string& entry) mutable
				{
					*out = entry;
					++out;
				});
		}

		inline std::vector<std::string> GlobEntriesVector(const std::string& key)
		{
			std::vector<std::string> retval;
			GlobEntriesIterator(key, std::back_inserter(retval));

			return retval;
		}

		void GlobValue(const std::string& value, const std::function<void(const std::string&)>& entryCallback);

		template<typename OutputIterator>
		inline void GlobValueIterator(const std::string& value, OutputIterator out)
		{
			GlobValue(value, [&out](const std::string& entry) mutable
				{
					*out = entry;
					++out;
				});
		}

		inline std::vector<std::string> GlobValueVector(const std::string& value)
		{
			std::vector<std::string> retval;
			GlobValueIterator(value, std::back_inserter(retval));

			return retval;
		}

		inline void AddMetaData(const std::string& key, const std::string& value)
		{
			m_metaDataEntries.insert({ key, value });
		}
	};
}

DECLARE_INSTANCE_TYPE(fx::ResourceMetaDataComponent);