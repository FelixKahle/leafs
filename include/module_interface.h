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

#ifndef FKL_MODULE_INTERFACE_H
#define FKL_MODULE_INTERFACE_H

namespace fkleafs
{
	/**
	 * Interface for all modules.
	 * Inherit from this to declare a module.
	 */
	class ModuleInterface
	{
	public:

		/**
		 * Note: Even though this is an interface class we need a virtual destructor here because modules are deleted via a pointer to this interface
		 */
		virtual ~ModuleInterface()
		{
		}

		/**
		 * Called right after the module DLL has been loaded and the module object has been created
		 */
		virtual void OnStartupModule()
		{
		}

		/**
		 * Called before the module is unloaded, right before the module object is destroyed.
		 */
		virtual void OnShutdownModule()
		{
		}
	};
}

#endif // !FKL_MODULE_INTERFACE_H