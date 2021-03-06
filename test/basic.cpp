#include <clangpp.hpp>
#include <vector>
#include <algorithm>

#define CHECK(...) if (!(__VA_ARGS__)) { printf("Failed: %s\n", #__VA_ARGS__); std::abort(); }

int main() {
    std::string dir = __FILE__;
    dir = dir.substr(0, dir.rfind('/')+1);

    clang::index idx{};
    auto tu = idx.parse_translation_unit(dir + "example.cpp");
    auto diags = tu.get_diagnostic();
    CHECK(diags.begin() == diags.end());
    std::vector<std::string> classes;
    std::vector<std::string> methods;
    tu.get_translation_unit_cursor().visit_children([&](clang::cursor c, clang::cursor)
    {
        if (c.get_kind() == CXCursor_StructDecl || c.get_kind() == CXCursor_ClassDecl)
        {
            classes.push_back(c.get_display_name().to_std_string());
            return CXChildVisit_Recurse;
        }
        else if (c.get_kind() == CXCursor_CXXMethod)
        {
            methods.push_back(c.get_display_name().to_std_string());
            return CXChildVisit_Continue;
        }
        return CXChildVisit_Recurse;
    });
    CHECK(classes.size() == 1);
    CHECK(methods.size() == 1);
    CHECK(classes[0] == "foo");
    CHECK(methods[0] == "method()");

    auto f = tu.get_file(dir + "example.cpp");
    auto start = tu.get_location(f, 1, 1);
    auto stop = tu.get_location(f, 7, 1);
    auto tokens = tu.tokenize({start, stop});
    for(std::string s:{"foo", "struct", "method"})
    {
        // printf("Token: %s\n", s.c_str());
        CHECK(std::any_of(tokens.begin(), tokens.end(), [&](clang::token t)
        {
            return t.get_spelling().to_std_string() == s;
        }));
        
    }
}
