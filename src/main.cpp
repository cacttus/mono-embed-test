// mono-embed-test
//
// Test C++/C# interactivity using Mono as a hypothetical game engine scripting language.
// Also test performance overhead of mono vs c++.
//
// Bugs:
//   mono_jit_exec produces an error running the application outside of the dev environment - Ubuntu.
//
// Not tested in Windows/Mac.
//
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <cstring>

#define Stz std::string("") +
#define BR2_USE_MONO
#define BR2_OS_LINUX

#if defined(BR2_USE_MONO)
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/threads.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/mono-config.h>
#include <mono/metadata/threads.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/mono-gc.h>
#include <mono/metadata/environment.h>
#include <mono/metadata/threads.h>
#include <mono/metadata/exception.h>
#endif

#if defined(BR2_OS_LINUX)
//This gets the OS name
#include <sys/utsname.h>
//Sort of similar to windows GetLastError
#include <sys/errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#endif

typedef std::string string_t;

float randomFloat() {
  static bool _frand_init = false;
  if (!_frand_init) {
    srand((unsigned int)18093050563);
    _frand_init = true;
  }
  return float(float(rand()) / float(RAND_MAX));
}

class vec3 {
public:
  float x, y, z;
  vec3() { x = y = z = 0; }
  vec3(float dx, float dy, float dz) {
    x = dx;
    y = dy;
    z = dz;
  }
  static vec3 random() {
    return vec3(randomFloat(), randomFloat(), randomFloat());
  }
  vec3& cross(const vec3& v1) {
    this->x = (y * v1.z) - (v1.y * z);
    this->y = (z * v1.x) - (v1.z * x);
    this->z = (x * v1.y) - (v1.x * y);
    return *this;
  }
  vec3& operator=(const vec3& v) {
    this->x = v.x;
    this->y = v.y;
    this->z = v.z;
    return *this;
  }
  vec3 operator+(const vec3& v) const {
    vec3 tmp = *this;
    tmp += v;
    return tmp;
  }
  vec3& operator+=(const vec3& v) {
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
  }
  vec3 operator-(const vec3& v) const {
    vec3 tmp = *this;
    tmp -= v;
    return tmp;
  }
  vec3& operator-=(const vec3& v) {
    x -= v.x;
    y -= v.y;
    z -= v.z;
    return *this;
  }
  vec3 operator/(const float& v) const {
    vec3 tmp = *this;
    tmp /= v;
    return tmp;
  }
  vec3& operator/=(const float& v) {
    x /= v;
    y /= v;
    z /= v;
    return *this;
  }
};

// Returns time equivalent to a number of frames at 60fps
#define FPS60_IN_MS(frames) ((int)((1.0f / 60.0f * frames) * 1000.0f))

void doHardWork_Frame(float f) {
  int ms = FPS60_IN_MS(f);
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void errorExit(const std::string& str) {
  std::cout << "Error: " << str << std::endl;
  std::cout << "Press any key to exit.. " << std::endl;
  std::cin.get();
  exit(1);
}

class Gu {
public:
  static void debugBreak() {
#if defined(BR2_OS_WINDOWS)
    DebugBreak();
#elif defined(BR2_OS_LINUX)
    raise(SIGTRAP);
#else
    OS_NOT_SUPPORTED_ERROR
#endif
  }
  static uint64_t getMicroSeconds() {
    int64_t ret;
    std::chrono::nanoseconds ns = std::chrono::high_resolution_clock::now().time_since_epoch();
    ret = std::chrono::duration_cast<std::chrono::microseconds>(ns).count();
    return ret;
  }
  static uint64_t getMilliSeconds() {
    return getMicroSeconds() / 1000;
  }
};

string_t executeReadOutput(const string_t& cmd) {
  string_t data = "";
#if defined(BR2_OS_LINUX)
  //This works only if VSCode launches the proper terminal (some voodoo back there);
  const int MAX_BUFFER = 8192;
  char buffer[MAX_BUFFER];
  std::memset(buffer, 0, MAX_BUFFER);
  string_t cmd_mod = Stz cmd + " 2>&1";  //redirect stderr to stdout

  FILE* stream = popen(cmd_mod.c_str(), "r");
  if (stream) {
    while (fgets(buffer, MAX_BUFFER, stream) != NULL) {
      data.append(buffer);
    }
    if (ferror(stream)) {
      std::cout << "Error executeReadOutput() " << std::endl;
    }
    clearerr(stream);
    pclose(stream);
  }
#else
  BRLogWarn("Tried to call invalid method on non-linux platform.");
  //Do nothing
#endif
  return data;
}

//Not static in the example.

MonoClass* get_class(const std::string& image_name, const std::string& namespace_name,
                     const std::string& class_name) {
  MonoImageOpenStatus status;
  MonoImage* mygame_image = mono_image_open(image_name.c_str(), &status);
  if (mygame_image == nullptr) {
    errorExit("MyGame dll not found.");
  }
  MonoClass* className = mono_class_from_name(mygame_image, namespace_name.c_str(), class_name.c_str());
  mono_image_close(mygame_image);  //This appears to b
  return className;
}
MonoMethod* find_static_method(const std::string& image_name, const std::string& namespace_name,
                               const std::string& class_name, const std::string& methodname) {
  // MonoImageOpenStatus status;
  // MonoImage* mygame_image = mono_image_open(image_name.c_str(), &status);
  // if (mygame_image == nullptr) {
  //   errorExit("MyGame dll not found.");
  // }
  // MonoClass* className = mono_class_from_name(mygame_image, namespace_name.c_str(), class_name.c_str());
  // mono_image_close(mygame_image);  //This appears to be alright.

  MonoClass* className = get_class(image_name, namespace_name, class_name);

  //[namespace.]classname:methodname[(args...)]

  MonoMethodDesc* desc;
  desc = mono_method_desc_new(methodname.c_str(), true);
  MonoMethod* return_method = mono_method_desc_search_in_class(desc, className);
  mono_method_desc_free(desc);
  if (return_method == nullptr) {
    errorExit(Stz "cound not locate Method: " + methodname);
  }
  std::cout << "method found: " << mono_method_full_name(return_method, true) << std::endl;

  return return_method;
}
/////////////////////////////////////////////////////////////////////////////////////////
MonoString* doComplexEngineWork() {
  return mono_string_new(mono_domain_get(), "Hello from C++!");
}
//This is a cache of the C# version of vec3. We could move this into a class. This is a little more tedious than LuaIntf so I'm not sure how we'll do this.
MonoClass* v3class_cached = nullptr;
MonoClassField* v3class_field_x_cached = nullptr;
MonoClassField* v3class_field_y_cached = nullptr;
MonoClassField* v3class_field_z_cached = nullptr;
void checkVec3MonoClassLoaded() {
  // Grab vec3 (C# version) if it isn't grabbed. Then get fields.
  if (v3class_cached == nullptr) {
    v3class_cached = get_class("./MyGame.dll", "MyGame", "vec3");
    if (v3class_cached == nullptr) {
      errorExit(Stz "Failed to find vec3 class in c#");
    }
    v3class_field_x_cached = mono_class_get_field_from_name(v3class_cached, "x");
    if (v3class_field_x_cached == nullptr) {
      errorExit(Stz "Failed to find vec3 x field in c#");
    }
    v3class_field_y_cached = mono_class_get_field_from_name(v3class_cached, "y");
    if (v3class_field_y_cached == nullptr) {
      errorExit(Stz "Failed to find vec3 y field in c#");
    }
    v3class_field_z_cached = mono_class_get_field_from_name(v3class_cached, "z");
    if (v3class_field_z_cached == nullptr) {
      errorExit(Stz "Failed to find vec3 z field in c#");
    }
  }
}
void unboxVec3(MonoObject* vec3_cs, vec3& vec3_cpp) {
  //Get a vec3 from mono
  mono_field_get_value(vec3_cs, v3class_field_x_cached, (void*)&vec3_cpp.x);
  mono_field_get_value(vec3_cs, v3class_field_y_cached, (void*)&vec3_cpp.y);
  mono_field_get_value(vec3_cs, v3class_field_z_cached, (void*)&vec3_cpp.z);
}
MonoObject* boxVec3(vec3& vec3_cpp) {
  //Send a vec3 to mono
  MonoObject* obj = mono_object_new(mono_domain_get(), v3class_cached);
  mono_field_set_value(obj, v3class_field_x_cached, (void*)&vec3_cpp.x);
  mono_field_set_value(obj, v3class_field_y_cached, (void*)&vec3_cpp.y);
  mono_field_set_value(obj, v3class_field_z_cached, (void*)&vec3_cpp.z);
  return obj;
}
void vec3CrossProductNoMonoOverhead(vec3& a, vec3& b, vec3& c){
  for (int i = 0; i < 1000; ++i) {
    c = a.cross(b);
  }
}
MonoObject* vec3CrossProduct(MonoObject* a_cs, MonoObject* b_cs) {
  //Simulate some complex engine work.
  checkVec3MonoClassLoaded();

  vec3 a, b;
  unboxVec3(a_cs, a);
  unboxVec3(a_cs, b);
  vec3 c;
  vec3CrossProductNoMonoOverhead(a,b,c);
  MonoObject* obj = boxVec3(c);

  return obj;
}

MonoObject* cppUnboxTest(MonoObject* vec3_cs) {
  checkVec3MonoClassLoaded();

  vec3 a;
  unboxVec3(vec3_cs, a);
  a.x += 1;
  a.y += 1;
  a.z += 1;
  MonoObject* obj = boxVec3(a);
  return obj;
}
void printcwd() {
  char* d = getcwd(NULL, 0);
  std::cout << "CWD:" << d << std::endl;
}
///////////////////////
int main(int argc, char** argv) {
  // Apparently you need pkg-config in the mono docs. I didn't use pkg-config in cmake but this should be noted.
  //# `pkg-config --cflags --libs mono-2`
  printcwd();

//Compile files.
#ifdef unix
  string_t out;
  std::cout << "Compiling hello" << std::endl;
  out = executeReadOutput("csc ./data/scripts/hello.cs -t:exe -out:./hello.exe");
  std::cout << "Compile Output: " << out << std::endl;
  if (out.find("error") != std::string::npos) {
    std::cout << " << compile error. See output. Press Enter to Continue." << std::endl;
    Gu::debugBreak();
  }
  std::cout << "Compiling MyGame" << std::endl;
  out = executeReadOutput("csc ./data/scripts/MyGame.cs -t:library -out:./MyGame.dll");
  std::cout << "Compile Output: " << out << std::endl;
  if (out.find("error") != std::string::npos) {
    std::cout << " << compile error. See output. Press Enter to Continue." << std::endl;
    Gu::debugBreak();
  }
#else
#pragma error("Unsupported operating system. Try to uncomment this #ifdef to see if it works in Windows.")
#endif

  // Note:
  // This is if you need to relocate mono to a custom directory ex. in Windows or with a build.
  //  mono_set_dirs("/usr/lib","/etc");

  std::cout << "Parsing config" << std::endl;
  mono_config_parse(NULL);

  //The API says specifically to use mono_jit_init and not anything else to initialize mono.
  //Mono does not like when you call jit_init more than once. Just once when you start your app.
  std::cout << "Executing jit_init" << std::endl;
  MonoDomain* main_domain = mono_jit_init("Dummy");
  std::cout << "  current heap used size: " << mono_gc_get_used_size() << std::endl;
  {
    // Note:
    // You cannot unload assemblies in C#.
    // To Prevent memory loss you can dynamically load your assembly in another AppDomain then release that domain.
    // This does not appear to work on Ubuntu without hitting some bugs in the mono runtime.
    printcwd();

    ///////////////////////////////////////////////////////////////////////////////////////////////
    //Test #1 -
    // {
    //   // This is the first basic test of executing an assembly as a standalone program.
    //   //  * Note Ubuntu, This test does not appear to work outside the dev environment filename != NULL .. ?.
    //   //  * Moving this test to the temporary appdomain causes further issues. mono_jit_exec should be avoided until we figure this out.
    //   std::cout << "Test #1 - Run an .exe print output to console in root domain" << std::endl;
    //   MonoAssembly* assembly = mono_domain_assembly_open(mono_domain_get(), "./hello.exe");
    //   if (!assembly) {
    //     errorExit(" Assembly not found.");
    //   }
    //   else {
    //     std::cout << " a. Assembly found.. " << mono_assembly_name_get_name(mono_assembly_get_name(assembly)) << std::endl;
    //   }
    //   std::cout << " b. Executing hello.exe assembly.." << std::endl;
    //   int retval = mono_jit_exec(mono_domain_get(), assembly, argc - 1, argv + 1);
    //   std::cout << " c. Done. Assembly returned " << retval << std::endl;
    // }
    ///////////////////////////////////////////////////////////////////////////////////////////////
    {
      std::cout << "Test #2 - Create new Appdomain" << std::endl;
      std::cout << "  Creating new domain, current heap used size: " << mono_gc_get_used_size() << std::endl;
      MonoDomain* temp_domain = mono_domain_create_appdomain((char*)"temp_domain", NULL);
      if (!mono_domain_set(temp_domain, 0)) {
        errorExit("  Failed to set the domain");
      }
      ////////////////////////////////////////////////////////////////////////////////////////
      const char* mygame_asm = "./MyGame.dll";
      MonoAssembly* assembly = mono_domain_assembly_open(mono_domain_get(), mygame_asm);
      if (!assembly) {
        errorExit(" Assembly not found.");
      }
      ////////////////////////////////////////////////////////////////////////////////////////
      {
        {
          std::cout << "Test #3 - Throw exception in C# and catch in C++" << std::endl;

          //Assemblies contain images. I'm not sure why you pass the filename if the assembly needs to be loaded anyway..?
          MonoMethod* method = find_static_method(mygame_asm, "MyGame", "MyClass", "MyGame.MyClass:OnStart()");
          MonoObject* exception = nullptr;
          MonoObject* result = mono_runtime_invoke(method, NULL, NULL, &exception);
          if (exception != nullptr) {
            errorExit(Stz " MyGame dll -> exception thrown: ??");  //How to get exception information from a MonoObject ?
          }
          mono_free_method(method);
          mono_gc_collect(mono_gc_max_generation());
        }
        ///////////////////////////////////////////////////////////////////////////////////////////////
        {
          std::cout << "Test #4 - Call C++ method from C#" << std::endl;
          //Assemblies contain images. I'm not sure why you pass the filename if the assembly needs to be loaded anyway..?
          MonoMethod* method = find_static_method(mygame_asm, "MyGame", "MyClass", "MyGame.MyClass:ExceptionTest()");
          MonoObject* exception = nullptr;
          MonoObject* result = mono_runtime_invoke(method, NULL, NULL, &exception);
          if (exception != nullptr) {
            //exception->message
            MonoException* e = mono_get_exception_runtime_wrapped(exception);
            //How to get the exception message?
            //MonoError
            std::cout << " Test Passed - exception thrown and caught in C++" << std::endl;
          }
          mono_free_method(method);
          mono_gc_collect(mono_gc_max_generation());
        }
        ///////////////////////////////////////////////////////////////////////////////////////////////
        {
          std::cout << "Test #5 - Call a C++ method from C# and return a C# class with the data." << std::endl;
          mono_add_internal_call("MyGame.MyClass::CppUnboxTest", reinterpret_cast<void*>(cppUnboxTest));
          MonoMethod* CSUnboxTest_method = find_static_method(mygame_asm, "MyGame", "MyClass", "MyGame.MyClass:CSUnboxTest()");
          MonoObject* exception = nullptr;
          MonoObject* result = nullptr;

          //a. Test OnUpdate once.
          std::cout << " Running test to send mono class to c++." << std::endl;
          result = mono_runtime_invoke(CSUnboxTest_method, NULL, NULL, NULL);
          if (exception != nullptr) {
            errorExit(Stz " MyGame dll -> exception thrown: ??");  //How to get exception information from a MonoObject ?
          }
          mono_free_method(CSUnboxTest_method);
          mono_gc_collect(mono_gc_max_generation());
        }
        ///////////////////////////////////////////////////////////////////////////////////////////////
        {
          std::cout << "Test 6 - Call a C++ method from C# and return a C# class with the data." << std::endl;
          //Holy shit it worked. I was using Namespace.Class:Method the signature but it must be N.C::M.
          mono_add_internal_call("MyGame.MyClass::Vec3CrossProduct", reinterpret_cast<void*>(vec3CrossProduct));

          MonoMethod* onupdate_method = find_static_method(mygame_asm, "MyGame", "MyClass", "MyGame.MyClass:OnUpdate()");
          MonoObject* exception = nullptr;
          MonoObject* result = nullptr;

          //a. Test OnUpdate once.
          std::cout << " Running OnUpdate once to verify it works." << std::endl;
          result = mono_runtime_invoke(onupdate_method, NULL, NULL, NULL);
          if (exception != nullptr) {
            errorExit(Stz " MyGame dll -> exception thrown: ??");  //How to get exception information from a MonoObject ?
          }

          //b. Run OnUpdate in our test game loop.
          std::cout << " Running OnUpdate for 13 milliseconds.." << std::endl;
          uint64_t tA, tB;

          tA = Gu::getMilliSeconds();
          int did_run = 0;
          while (true) {
            tB = Gu::getMilliSeconds() - tA;
            if (tB > 13) {
              break;
            }
            result = mono_runtime_invoke(onupdate_method, NULL, NULL, NULL);
            if (exception != nullptr) {
              errorExit(Stz " MyGame dll -> exception thrown: ??");  //How to get exception information from a MonoObject ?
            }
            did_run++;
          }
          std::cout << " Ran OnUpdate " << did_run << " times (C#->C++->C#)." << std::endl;
          
          std::cout << " (performance) Running same method without mono overhead " << std::endl;
          int did_run2=0;
          tA = Gu::getMilliSeconds();
          while (true) {
            tB = Gu::getMilliSeconds() - tA;
            if (tB > 13) {
              break;
            }
            vec3 a(1,0,0), b(0,1,0), c(0,0,0);
            vec3CrossProductNoMonoOverhead(a,b,c);
            did_run2++;
          }
          std::cout << " Ran vec3CrossProductNoMonoOverhead " << did_run2 << " times (C++ only)." << std::endl;
          std::cout << " Performance increase in c++ only version: %" << 100 - ((float)did_run / (float)did_run2 * 100) << std::endl;

          mono_free_method(onupdate_method);

          //c. Run OnExit to print the output in C#
          MonoMethod* onexit_method = find_static_method(mygame_asm, "MyGame", "MyClass", "MyGame.MyClass:OnExit()");
          exception = nullptr;
          result = mono_runtime_invoke(onexit_method, NULL, NULL, &exception);
          if (exception != nullptr) {
            errorExit(Stz " MyGame dll -> exception thrown: ??");  //How to get exception information from a MonoObject ?
          }
          mono_free_method(onexit_method);
          mono_gc_collect(mono_gc_max_generation());
        }
      }
      ///////////////////////////////////////////////////////////////////////////////////////////////
      std::cout << "Test #7 - Unload the temporary app domain to free memory." << std::endl;
      MonoDomain* toUnload = mono_domain_get();
      if (toUnload != mono_get_root_domain()) {
        if (!mono_domain_set(mono_get_root_domain(), true)) {
          errorExit("Failed to set the domain");
        }
        std::cout << " Unloading new Appdomain, current heap used size: " << mono_gc_get_used_size() << std::endl;
        mono_domain_unload(toUnload);
        std::cout << " Unloaded Appdomain, current heap used size: " << mono_gc_get_used_size() << std::endl;
      }
    }
  }
  std::cout << "running >> mono_domain_set" << std::endl;
  mono_domain_set(main_domain, false);
  std::cout << "running >> mono_jit_cleanup" << std::endl;
  mono_jit_cleanup(main_domain);

  /// End of Mono Test
  std::cout << "Done. Press any key to exit.." << std::endl;
  std::cin.get();

  return 0;
}