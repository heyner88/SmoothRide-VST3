#include "../Source/PresetWorkflowPolicy.h"
#include <array>
#include <cassert>
#include <iostream>
#include <string>

int main() {
    using namespace smoothride::preset;
    assert(!validName(0));
    assert(validName(21));
    assert(validName(40));
    assert(!validName(41));
    assert(mayCreate(5, false));
    assert(!mayCreate(5, true)); // Save never silently overwrites.
    assert(!mayCreate(0, false));
    std::array<float, 13> saved{};
    auto current = saved;
    assert(!isModified(saved, current));
    current[6] = 0.01f; // Tiny quantization difference stays clean.
    assert(!isModified(saved, current));
    current[6] = 1.0f; // Mix edited -> modified.
    assert(isModified(saved, current));
    saved = current; // Update: saved reference matches current.
    assert(!isModified(saved, current));
    current[6] = 0.0f;
    assert(isModified(saved, current));
    current = saved; // Revert: return to saved reference.
    assert(!isModified(saved, current));
    std::cout << "Preset workflow policy tests passed\n";
}
