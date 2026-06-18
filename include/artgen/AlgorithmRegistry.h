#pragma once
#include "artgen/IAlgorithm.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace artgen {

struct SceneConfig; // forward declaration — full type available in .cpp files

// ── Algorithm factory registry ────────────────────────────────────────────────
//
// Maps algorithm names to factory functions that produce fully-configured
// IAlgorithm instances from a SceneConfig.  The built-in algorithms are
// registered lazily on first use.  Third-party code can register additional
// algorithms before calling SceneConfig::create_algorithm().
//
// Usage:
//   AlgorithmRegistry::register_algo("my_algo",
//       [](const SceneConfig& cfg) { ... return make_unique<MyAlgo>(); });
//   auto algo = AlgorithmRegistry::create("my_algo", cfg);

class AlgorithmRegistry {
public:
    using Factory = std::function<std::unique_ptr<IAlgorithm>(const SceneConfig&)>;

    // Register a named factory (overwrites any existing entry with the same name).
    static void register_algo(const std::string& name, Factory f);

    // True if the name has been registered.
    static bool has(const std::string& name);

    // Sorted list of all registered algorithm names.
    static std::vector<std::string> names();

    // Create a configured algorithm by name.  Returns nullptr if not found.
    static std::unique_ptr<IAlgorithm> create(const std::string& name,
                                               const SceneConfig& cfg);

private:
    // Trigger lazy registration of built-in algorithms (thread-safe).
    static void ensure_registered();
};

} // namespace artgen
