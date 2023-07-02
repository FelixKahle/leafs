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
			m_mutex.Lock();
			m_modules.clear();
			m_mutex.Unlock();
		}

		/**
		 * Tests whether a module is loaded.
		 *
		 * @param info The module info to check loading state.
		 * @return True if the module is loaded, false otherwise.
		 */
		inline bool IsModuleLoaded(const ModuleInfo &info) const
		{
			m_mutex.ReaderLock();
			const bool result = m_modules.contains(info);
			m_mutex.ReaderUnlock();
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

		/**
		 * Returns the count of currently loaded modules.
		 *
		 * @return The count of currently loaded modules.
		 */
		inline int ModuleCount() const
		{
			m_mutex.ReaderLock();
			const int size = m_modules.size();
			m_mutex.ReaderUnlock();
			return size;
		}

		/**
		 * Adds a module to the manager.
		 *
		 * @tparam Module The module type.
		 * @param module_ptr Pointer to the module.
		 * @param info The module info.
		 */
		template <typename Module>
		bool AddModule(std::shared_ptr<Module> module_ptr = ModuleCreator<Module>::Create(), const ModuleInfo info = ModuleInfo::GetModuleInfo<Module>())
		{
			// Required that Module is derived from ModuleInterface.
			static_assert(std::is_base_of<ModuleInterface, Module>::value, "Any Module should be derived from ModuleInterface");

			// Catch any invalid states.
			// NOTE: It is impossible to catch an invalid pointer. Only a nullptr check is possible.
			if (IsModuleLoaded(info))
			{
				LOG(ERROR) << "The module: " << info.ModuleName() << " is already loaded";
				return false;
			}
			if (!module_ptr)
			{
				LOG(ERROR) << "Nullptr module cannot be added";
				return false;
			}

			std::shared_ptr<ModuleInterface> module_interface_ptr = std::static_pointer_cast<ModuleInterface>(module_ptr);
			module_interface_ptr->OnStartupModule();

			m_mutex.Lock();
			m_modules.emplace(info, module_interface_ptr);
			m_mutex.Unlock();

			return true;
		}

		/**
		 * Removes a module.
		 * 
		 * @tparam Module The module type that should be removed.
		 * @return True if the remove operation was successful, false otherwise.
		 */
		template<typename Module>
		bool RemoveModule(const ModuleInfo info = ModuleInfo::GetModuleInfo<Module>())
		{
			// Required that Module is derived from ModuleInterface.
			static_assert(std::is_base_of<ModuleInterface, Module>::value, "Any Module should be derived from ModuleInterface");

			return RemoveModule(info);
		}

		/**
		 * Removes a module.
		 *
		 * @param info The info about the module that should be removed.
		 * @return True if the remove operation was successful, false otherwise.
		 */
		bool RemoveModule(const ModuleInfo& info)
		{
			if (!IsModuleLoaded(info))
			{
				LOG(ERROR) << "The module: " << info.ModuleName() << " has not been loaded and connot be removed";
				return false;
			}

			std::shared_ptr<ModuleInterface> module_interface_ptr = GetModuleInterfaceSharedPtr(info);
			// Should never happen, check to be sure.
			if (!module_interface_ptr)
			{
				LOG(ERROR) << "Obtained nullptr from module: " << info.ModuleName();
				return false;
			}

			module_interface_ptr->OnShutdownModule();

			m_mutex.Lock();
			m_modules.erase(info);
			m_mutex.Unlock();

			return true;
		}

		/**
		 * Returns a pointer to the ModuleInterface of a given ModuleInfo.
		 *
		 * @param info The ModuleInfo.
		 * @return Weak pointer to the module interface.
		 */
		std::weak_ptr<ModuleInterface> GetModuleInterfacePtr(const ModuleInfo &info) const
		{
			if (!IsModuleLoaded(info))
			{
				LOG(ERROR) << "The module: " << info.ModuleName() << " has not been loaded. Nullptr is returned.";
				return std::weak_ptr<ModuleInterface>();
			}

			m_mutex.ReaderLock();
			const std::weak_ptr<ModuleInterface> result = m_modules.at(info);
			m_mutex.ReaderUnlock();

			return result;
		}

		/**
		 * Returns a pointer to the module interface of a given type.
		 *
		 * @tparam Module The module type.
		 * @param The module info.
		 * @return Weak pointer to the module interface.
		 */
		template <typename Module>
		std::weak_ptr<ModuleInterface> GetModuleInterfacePtr(const ModuleInfo info = ModuleInfo::GetModuleInfo<Module>()) const
		{
			// Required that Module is derived from ModuleInterface.
			static_assert(std::is_base_of<ModuleInterface, Module>::value, "Any Module should be derived from ModuleInterface");

			return GetModuleInterfacePtr(info);
		}

		/**
		 * Returns the pointer to a module of a given type.
		 * 
		 * @tparam Module The type of the module.
		 * @param The info of the module.
		 * @return Weak pointer to the module.
		 */
		template <typename Module>
		std::weak_ptr<Module> GetModulePtr(const ModuleInfo info = ModuleInfo::GetModuleInfo<Module>()) const
		{
			// Required that Module is derived from ModuleInterface.
			static_assert(std::is_base_of<ModuleInterface, Module>::value, "Any Module should be derived from ModuleInterface");

			std::shared_ptr<ModuleInterface> module_ptr = GetModuleInterfaceSharedPtr(info);
			return std::weak_ptr<Module>(std::static_pointer_cast<Module>(module_ptr));
		}

	private:
		/**
		 * Returns a shared pointer to the module interface of a ModuleInfo.
		 * Internal use only, because we dont want to extend the lifetime of any modules
		 * because the user forgot to clear a shared_ptr.
		 * 
		 * @param info The ModuleInfo.
		 * @return pointer to the ModuleInterface.
		 */
		std::shared_ptr<ModuleInterface> GetModuleInterfaceSharedPtr(const ModuleInfo& info) const
		{
			if (!IsModuleLoaded(info))
			{
				LOG(ERROR) << "The module: " << info.ModuleName() << " has not been loaded. Nullptr is returned.";
				return nullptr;
			}

			m_mutex.ReaderLock();
			const std::shared_ptr<ModuleInterface> result = m_modules.at(info);
			m_mutex.ReaderUnlock();

			return result;
		}

	private:
		/**
		 * Hashmap that holds all modules.
		 * We use the ModuleInfo class as keys.
		 */
		absl::flat_hash_map<ModuleInfo, std::shared_ptr<ModuleInterface>, ModuleInfo::ModuleInfoHash, ModuleInfo::ModuleInfoEqual> m_modules;

		/**
		 * Mutex used to lock the m_modules variable.
		 */
		mutable absl::Mutex m_mutex;
	};

	/**
	 * Used to create new module instances.
	 *
	 * @tparam Module the module type to create.
	 */
	template <typename Module>
	class ModuleCreator
	{
	public:
		// Required that Module is derived from ModuleInterface.
		static_assert(std::is_base_of<ModuleInterface, Module>::value, "Any Module should be derived from ModuleInterface");

		/**
		 * Returns a new instance of the given module type.
		 */
		static std::shared_ptr<Module> Create()
		{
			return std::make_shared<Module>();
		}
	};

	template <typename Module>
	class StaticallyLoadedModuleRegistrant
	{
	public:
		// Required that Module is derived from ModuleInterface.
		static_assert(std::is_base_of<ModuleInterface, Module>::value, "Any Module should be derived from ModuleInterface");

		StaticallyLoadedModuleRegistrant()
		{
			LOG(INFO) << "Statically register module: " << ModuleInfo::GetModuleInfo<Module>().ModuleName();
			// Add the module to the Manager.
			ModuleManager::Get().AddModule<Module>();
		}
	};
}

#define FKL_MODULE_MANAGER() fkleafs::ModuleManager::Get()

#define FKL_REGISTER_MODULE(ModuleType)                                                                         \
	namespace                                                                                                   \
	{                                                                                                           \
		fkleafs::StaticallyLoadedModuleRegistrant<ModuleType> statically_loaded_module_registrant_##ModuleType; \
	}

#define FKL_INJECT_MODULE(ModuleType, Getter)                            \
public:                                                                  \
	std::shared_ptr<ModuleType> Getter() const                           \
	{                                                                    \
		return fkleafs::ModuleManager::Get().GetModulePtr<ModuleType>(); \
	}                                                                    \
			                                                             \
private:

#endif // !FKL_MODULE_MANAGER_H