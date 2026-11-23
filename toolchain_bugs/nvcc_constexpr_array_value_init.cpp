// nvcc: a constexpr array of aggregates value-initialized with {} is not a
// constant expression when the aggregate also has a member with a default
// member initializer ("access to uninitialized subobject").
//
// e[1]{} value-initializes the array, so e[0].open is 0; nvcc's front end
// skips that zero-initialization when entry holds spec, whose member has a
// default initializer. Without spec, or with a single entry instead of an
// array, nvcc accepts it. Valid C++: g++ 13.3 and clang++ 23.1.6 accept it
// with -std=c++17 or c++20 -pedantic-errors -Wall -Wextra. Host code only,
// no device code needed.
//
// ffmt hits it in print_string's ph_entry phs[MAX_PH]{} (ph_entry holds a
// format_spec): every test of the nvcc ids (50-53) fails to compile.
//
// Seen with nvcc 12.9, 13.2 and 13.3 (V13.3.73), host g++ 13.3.
//
//   nvcc -std=c++20 -x cu -c nvcc_constexpr_array_value_init.cpp
//
// Expected: compiles. nvcc: "error: expression must have a constant value".
struct spec { char c = 0; };
struct entry { unsigned char open; spec s; };
int main() { constexpr entry e[1]{}; return e[0].open; }
