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

#include "leafs.h"

class ModuleA : FKL_MODULE_INTERFACE
{
public:
	virtual void OnStartupModule() override
	{
		LOG(INFO) << "Startup A";
	}

	virtual void OnShutdownModule() override
	{
		LOG(INFO) << "Shutdown A";
	}

	void Greet()
	{
		LOG(INFO) << "Hello from A";
	}
};
FKL_REGISTER_MODULE(ModuleA)

class ModuleB : FKL_MODULE_INTERFACE
{
public:

	FKL_INJECT_MODULE(ModuleA, GetModuleA)

	virtual void OnStartupModule() override
	{
		LOG(INFO) << "Startup B";

		FKL_REQUIRE_MODULE(ModuleA);

		GetModuleA().lock()->Greet();
	}

	virtual void OnShutdownModule() override
	{
		LOG(INFO) << "Shutdown B";
	}
};
FKL_REGISTER_MODULE(ModuleB)

int main()
{
	FKL_LOAD_MODULE(ModuleB);
	return 0;
}