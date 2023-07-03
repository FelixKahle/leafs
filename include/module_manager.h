// Copyright 2023 Felix Kahle.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef FKL_MODULE_MANAGER_H
#define FKL_MODULE_MANAGER_H

#include <memory>
#include <functional>

#include "absl/container/flat_hash_map.h"
#include "absl/base/log_severity.h"
#include "absl/log/log.h"
#include "absl/synchronization/mutex.h"

#include "module_info.h"
#include "module_interface.h"

namespace fkleafs
{
	/**
	 * Manages all modules.
	 */
	class ModuleManager
	{
	private:
		/**
		 * Private constructor for the singleton pattern. 
		 */
		ModuleManager()
		{
		}

	public:
		/**
		 * Getter for the singleton of the ModuleManager.
		 *
		 * @return The singleton ModuleManager.
		 */
		static ModuleManager &Get();

		/**
		 * Destructor.
		 */
		~ModuleManager()
		{
			TearDown();
		}

		/**
		 * Deletes all smart pointers to modules. 
		 */
		void TearDown()
		{
			m_modules_mutex.Lock();
			for (auto& iterator : m_modules)
			{
				iterator.second->OnShutdownModule();
			}
			m_modules.clear();
			m_modules_mutex.Unlock();
		}

		/**
		 * Returns the count of currently loaded modules.
		 *
		 * @return The count of currently loaded modules.
		 */
		inline int ModuleCount() const
		{
			m_modules_mutex.ReaderLock();
			const int size = m_modules.size();
			m_modules_mutex.ReaderUnlock();
			return size;
		}

		/**
		 * Tests whether a module is loaded.
		 *
		 * @param info The module info to check loading state.
		 * @return True if the module is loaded, false otherwise.
		 */
		inline bool IsModuleLoaded(const ModuleInfo &info) const
		{
			m_modules_mutex.ReaderLock();
			const bool result = m_modules.contains(info);
			m_modules_mutex.ReaderUnlock();
			return result;
		}

		/**
		 * Tests whether a module is loaded.
		 *
		 * @tparam Module The module type to check loading state.
		 * @param info The module info to check loading state.
		 * @return True if the module is loaded, false otherwise.
		 */
		template <typename Module>
		inline bool IsModuleLoaded(const ModuleInfo info = ModuleInfo::GetModuleInfo<Module>()) const
		{
			// Required that Module is derived from ModuleInterface.
			static_assert(std::is_base_of<ModuleInterface, Module>::value, "Any Module should be derived from ModuleInterface");

			return IsModuleLoaded(info);
		}

		inline bool IsModuleRegistered(const ModuleInfo& info) const
		{
			m_statically_registered_modules_mutex.ReaderLock();
			const bool result = m_statically_registered_modules.contains(info);
			m_statically_registered_modules_mutex.ReaderUnlock();
			return result;
		}

		template<typename Module>
		inline bool IsModuleRegistered(const ModuleInfo info = ModuleInfo::GetModuleInfo<Module>()) const
		{
			// Required that Module is derived from ModuleInterface.
			static_assert(std::is_base_of<ModuleInterface, Module>::value, "Any Module should be derived from ModuleInterface");

			return IsModuleRegistered(info);
		}

		bool RegisterModule(std::function<std::shared_ptr<ModuleInterface>()> module_creator_function, const ModuleInfo& info)
		{
			if (IsModuleRegistered(info))
			{
				LOG(ERROR) << "The module: " << info.ModuleName() << " is already registered";
				return false;
			}

			m_statically_registered_modules_mutex.Lock();
			m_statically_registered_modules.emplace(info, module_creator_function);
			m_statically_registered_modules_mutex.Unlock();
			return true;
		}

		template<typename Module>
		inline bool RegisterModule(std::function<std::shared_ptr<ModuleInterface>()> module_creator_function = []() -> std::shared_ptr<ModuleInterface>
			{ 
				return StaticallyLinkedModuleCreator<Module>::CreateModuleInterface(); 
			}, const ModuleInfo info = ModuleInfo::GetModuleInfo<Module>())
		{
			// Required that Module is derived from ModuleInterface.
			static_assert(std::is_base_of<ModuleInterface, Module>::value, "Any Module should be derived from ModuleInterface");

			return RegisterModule(module_creator_function, info);
		}

		bool LoadModule(const ModuleInfo& info)
		{
			if (IsModuleLoaded(info))
			{
				LOG(ERROR) << "The module: " << info.ModuleName() << " is already loaded";
				return false;
			}

			if (!IsModuleRegistered(info))
			{
				LOG(ERROR) << "The module: " << info.ModuleName() << " is not registered and cannot be loaded";
				return false;
			}

			m_statically_registered_modules_mutex.ReaderLock();
			std::shared_ptr<ModuleInterface> module_ptr = m_statically_registered_modules.at(info)();
			m_statically_registered_modules_mutex.ReaderUnlock();

			module_ptr->OnStartupModule();

			m_modules_mutex.Lock();
			m_modules.emplace(info, module_ptr);
			m_modules_mutex.Unlock();
			return true;
		}

		template<typename Module>
		bool LoadModule(const ModuleInfo info = ModuleInfo::GetModuleInfo<Module>())
		{
			// Required that Module is derived from ModuleInterface.
			static_assert(std::is_base_of<ModuleInterface, Module>::value, "Any Module should be derived from ModuleInterface");

			return LoadModule(info);
		}

		bool UnloadModule(const ModuleInfo& info)
		{
			if (!IsModuleLoaded(info))
			{
				LOG(ERROR) << "The module: " << info.ModuleName() << " is not loaded and cannot be unloaded";
				return false;
			}

			m_modules_mutex.Lock();
			m_modules.at(info)->OnShutdownModule();
			m_modules.erase(info);
			m_modules_mutex.Unlock();

			return true;
		}

		template<typename Module>
		bool UnloadModule(const ModuleInfo info = ModuleInfo::GetModuleInfo<Module>())
		{
			// Required that Module is derived from ModuleInterface.
			static_assert(std::is_base_of<ModuleInterface, Module>::value, "Any Module should be derived from ModuleInterface");

			return UnloadModule(info);
		}

		std::weak_ptr<ModuleInterface> GetModuleInterfacePtr(const ModuleInfo& info)
		{
			if (!IsModuleLoaded(info))
			{
				LOG(ERROR) << "The module: " << info.ModuleName() << " is not loaded";

				// Try to recover from the error and attempt to load the module.
				if (!LoadModule(info))
				{
					LOG(ERROR) << "Failed to load module: " << info.ModuleName() << ". Nullptr is returned";
					return std::weak_ptr<ModuleInterface>();
				}
			}

			m_modules_mutex.ReaderLock();
			std::weak_ptr<ModuleInterface> result = m_modules.at(info);
			m_modules_mutex.ReaderUnlock();
			return result;
		}

		template<typename Module>
		std::weak_ptr<ModuleInterface> GetModuleInterfacePtr(const ModuleInfo info = ModuleInfo::GetModuleInfo<Module>())
		{
			// Required that Module is derived from ModuleInterface.
			static_assert(std::is_base_of<ModuleInterface, Module>::value, "Any Module should be derived from ModuleInterface");

			return GetModuleInterfacePtr(info);
		}

		template<typename Module>
		std::weak_ptr<Module> GetModulePtr(const ModuleInfo info = ModuleInfo::GetModuleInfo<Module>())
		{
			// Required that Module is derived from ModuleInterface.
			static_assert(std::is_base_of<ModuleInterface, Module>::value, "Any Module should be derived from ModuleInterface");

			std::shared_ptr<ModuleInterface> module_ptr = GetModuleInterfacePtr(info).lock();
			return std::static_pointer_cast<Module>(module_ptr);
		}

	private:

		/**
		 * Hashmap that holds all modules.
		 * We use the ModuleInfo class as keys.
		 */
		absl::flat_hash_map<ModuleInfo, std::shared_ptr<ModuleInterface>, ModuleInfo::ModuleInfoHash, ModuleInfo::ModuleInfoEqual> m_modules;

		/**
		 * All registered modules.
		 * NOTE: A module can be registered but no loaded.
		 */
		absl::flat_hash_map<ModuleInfo, std::function<std::shared_ptr<ModuleInterface>()>, ModuleInfo::ModuleInfoHash, ModuleInfo::ModuleInfoEqual> m_statically_registered_modules;

		/**
		 * Mutex used to lock the m_modules variable.
		 */
		mutable absl::Mutex m_modules_mutex;

		/**
		 * Mutex used to lock the m_statically_registered_modules variable.
		 */
		mutable absl::Mutex m_statically_registered_modules_mutex;
	};

	template<typename Module>
	class StaticallyLinkedModuleCreator
	{
	public:

		static std::shared_ptr<ModuleInterface> CreateModuleInterface()
		{
			return std::static_pointer_cast<ModuleInterface>(CreateModule());
		}

		static std::shared_ptr<Module> CreateModule()
		{
			return std::make_shared<Module>();
		}
	};

	template<typename Module>
	class StaticallyLinkedModuleRegistrant
	{
	public:
		StaticallyLinkedModuleRegistrant()
		{
			ModuleManager::Get().RegisterModule<Module>();
		}
	};
}

#define FKL_MODULE_INTERFACE public fkleafs::ModuleInterface
#define FKL_MODULE_MANAGER() fkleafs::ModuleManager::Get()
#define FKL_REGISTER_MODULE(ModuleType) \
	namespace \
	{ \
		fkleafs::StaticallyLinkedModuleRegistrant<ModuleType> statically_linked_module_registrant_##ModuleType; \
	}
#define FKL_REQUIRE_MODULE(ModuleType) FKL_MODULE_MANAGER().LoadModule<ModuleType>()
#define FKL_INJECT_MODULE(ModuleType, GetterName) \
	std::weak_ptr<ModuleType> GetterName() const \
	{ \
		return FKL_MODULE_MANAGER().GetModulePtr<ModuleType>(); \
	}
#define FKL_LOAD_MODULE(ModuleType) FKL_MODULE_MANAGER().LoadModule<ModuleType>()

#endif // !FKL_MODULE_MANAGER_H