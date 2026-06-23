#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <string>

int main(int argc, char* argv[]) {
    // Get OBT_STAGE from environment
    const char* obt_stage = getenv("OBT_STAGE");
    if (!obt_stage) {
        fprintf(stderr, "Error: OBT_STAGE environment variable not set\n");
        return 1;
    }
    
    // Build DYLD_LIBRARY_PATH (OBT stage only).
    // Homebrew (/opt/homebrew/lib) deliberately does NOT go here -- it goes on
    // DYLD_FALLBACK_LIBRARY_PATH below. DYLD_LIBRARY_PATH is an override searched
    // *before* a dylib's install name, so listing Homebrew here lets Homebrew's
    // Mesa libGL.dylib hijack the system OpenGL.framework's internal libGL by leaf
    // name; on macOS 26 that breaks CGL symbol resolution (e.g. CGLGetCurrentContext)
    // for any dylib that links system OpenGL. As a fallback it is only consulted
    // when the install name cannot be resolved, so system frameworks win.
    std::string dyld_path = std::string(obt_stage) + "/lib";

    // Check if DYLD_LIBRARY_PATH already exists and append if so
    const char* existing_dyld = getenv("DYLD_LIBRARY_PATH");
    if (existing_dyld && strlen(existing_dyld) > 0) {
        dyld_path = dyld_path + ":" + existing_dyld;
    }

    // Set the environment variable
    setenv("DYLD_LIBRARY_PATH", dyld_path.c_str(), 1);

    // Homebrew libraries as a fallback (searched after install names, not before).
    std::string fallback_path = std::string(obt_stage) + "/lib:/opt/homebrew/lib";
    const char* existing_fallback = getenv("DYLD_FALLBACK_LIBRARY_PATH");
    if (existing_fallback && strlen(existing_fallback) > 0) {
        fallback_path = fallback_path + ":" + existing_fallback;
    }
    setenv("DYLD_FALLBACK_LIBRARY_PATH", fallback_path.c_str(), 1);
    
    // Build path to ork.python
    std::string python_path = std::string(obt_stage) + "/bin/ork.python";
    
    // Build argument list for execv
    std::vector<char*> exec_args;
    exec_args.push_back(strdup("ork.python"));
    
    // Add all original arguments (script name and any additional args)
    for (int i = 1; i < argc; i++) {
        exec_args.push_back(argv[i]);
    }
    exec_args.push_back(nullptr);
    
    // Execute ork.python with the modified environment
    execv(python_path.c_str(), exec_args.data());
    
    // If we get here, execv failed
    perror("execv failed");
    fprintf(stderr, "Failed to execute: %s\n", python_path.c_str());
    return 1;
}