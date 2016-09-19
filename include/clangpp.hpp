
#ifndef LIBCLANGPP_CLANGPP_H
#define LIBCLANGPP_CLANGPP_H

#include <memory>
#include <type_traits>
#include <iterator>
#include <clang-c/Index.h>
#include <clang-c/Documentation.h>
#include <clang-c/CXCompilationDatabase.h>


namespace clang {

namespace detail {

template<class T, class Deleter>
using unique_ptr = std::unique_ptr<typename std::remove_pointer<T>::type, Deleter>;

template<class T>
using shared_ptr = std::shared_ptr<typename std::remove_pointer<T>::type>;

template<class F>
struct index_iterator
{
    int index;
    F* f;

    using difference_type = std::ptrdiff_t;
    using reference = decltype((*f)(1));
    using value_type = typename std::remove_reference<reference>::type;
    using pointer = typename std::add_pointer<value_type>::type;
    using iterator_category = std::input_iterator_tag;

    index_iterator(int i, F& fun) : index(i), f(&fun)
    {}

    index_iterator& operator+=(int n)
    {
        index += n;
        return *this;
    }

    index_iterator& operator-=(int n)
    {
        index += n;
        return *this;
    }

    int& operator++()
    {
        index++;
        return *this;
    }

    int& operator--()
    {
        index--;
        return *this;
    }

    int operator++(int)
    {
        index_iterator it = *this;
        index++;
        return it;
    }

    int operator--(int)
    {
        index_iterator it = *this;
        index--;
        return it;
    }
    // TODO: operator->
    reference operator*() const
    {
        return f(index);
    }
};

template<class F>
inline index_iterator<F>
operator +(index_iterator<F> x, index_iterator<F> y)
{
    return index_iterator<F>(x.index + y.index, x.f);
}

template<class F>
inline index_iterator<F>
operator -(index_iterator<F> x, index_iterator<F> y)
{
    return index_iterator<F>(x.index - y.index, x.f);
}

template<class F>
inline bool
operator ==(index_iterator<F> x, index_iterator<F> y)
{
    return x.index == y.index;
}

template<class F>
inline bool
operator !=(index_iterator<F> x, index_iterator<F> y)
{
    return x.index != y.index;
}

template<class F>
struct index_range
{
    F f;
    int start, stop;
    index_range(F f, int start, int stop) 
    : f(f), start(start), stop(stop)
    {}

    using iterator = index_iterator<F>;
    using const_iterator = index_iterator<F>;

    long size() const
    {
        return stop - start;
    }

    bool empty() const
    {
        return start == stop;
    }

    iterator begin()
    {
        return iterator(start, f);
    }

    iterator end()
    {
        return iterator(stop, f);
    }
};

template<class F>
index_range<F> make_index_range(int start, int stop, F f)
{
    return index_range<F>(f, start, stop);
}

}

struct string
{
    CXString self;
    string()
    {}
    string(CXString s) : self(s)
    {}
    string(string&& rhs) : self(rhs.self)
    {
        rhs.self.data = nullptr;
    }
    string& operator=(string rhs)
    {
        std::swap(rhs.self, this->self);
        return *this;
    }
    string& operator=(CXString rhs)
    {
        std::swap(rhs, this->self);
        return *this;
    }
    string(const string&)=delete;
    ~string()
    {
        if (self.data != nullptr) clang_disposeString(self);
    }
    const char * c_str() const
    {
        return clang_getCString(self);
    }

    std::string to_std_string() const
    {
        return this->c_str();
    }
};

class string_view
{
    const char * s;
public:
    string_view() : s(nullptr)
    {}
    string_view(const char * s) : s(s)
    {}

    string_view(const std::string& s) : s(s.c_str())
    {}

    string_view(const string& s) : s(s.c_str())
    {}

    const char * c_str() const
    {
        return s;
    }
};

struct file
{
    CXFile self;
    file()
    {}
    file(CXFile s) : self(s)
    {}
    string get_file_name()
    {
        return clang_getFileName(self);
    }
    time_t get_file_time()
    {
        return clang_getFileTime(self);
    }
    CXFileUniqueID get_file_unique_id()
    {
        CXFileUniqueID result;
        int err = clang_getFileUniqueID(self, &result);
        if (err != 0) throw std::runtime_error("Unique ID failed");
        return result;
    }
};
struct file_location : file
{
    unsigned line;
    unsigned column;
    unsigned offset;
};

struct source_location
{
    CXSourceLocation self;
    source_location() : self(clang_getNullLocation())
    {}
    source_location(CXSourceLocation l) : self(l)
    {}
    unsigned equal_locations(source_location loc2)
    {
        return clang_equalLocations(self, loc2.self);
    }
    int is_in_system_header()
    {
        return clang_Location_isInSystemHeader(self);
    }
    int is_from_main_file()
    {
        return clang_Location_isFromMainFile(self);
    }

    void get_presumed_location(CXString * filename, unsigned * line, unsigned * column)
    {
        clang_getPresumedLocation(self, filename, line, column);
    }
    file_location get_expansion_location()
    {
        file_location result;
        clang_getExpansionLocation(self, &result.self, &result.line, &result.column, &result.offset);
        return result;
    }
    file_location get_instantiation_location()
    {
        file_location result;
        clang_getInstantiationLocation(self, &result.self, &result.line, &result.column, &result.offset);
        return result;
    }
    file_location get_spelling_location()
    {
        file_location result;
        clang_getSpellingLocation(self, &result.self, &result.line, &result.column, &result.offset);
        return result;
    }
    file_location get_file_location()
    {
        file_location result;
        clang_getFileLocation(self, &result.self, &result.line, &result.column, &result.offset);
        return result;
    }
};
struct source_range
{
    CXSourceRange self;
    source_range() : self(clang_getNullRange())
    {}
    source_range(CXSourceRange r) : self(r)
    {}
    source_range(source_location start, source_location end) : self(clang_getRange(start.self, end.self))
    {}
    unsigned equal_ranges(source_range range2)
    {
        return clang_equalRanges(self, range2.self);
    }
    int is_null()
    {
        return clang_Range_isNull(self);
    }
    source_location get_range_start()
    {
        return clang_getRangeStart(self);
    }
    source_location get_range_end()
    {
        return clang_getRangeEnd(self);
    }
};

struct token
{
    CXToken self;
    CXTokenKind get_token_kind()
    {
        return clang_getTokenKind(self);
    }
};

struct fix_it
{
    string replacement;
    source_range range;
};

struct diagnostic;

struct diagnostic_set
{
    detail::unique_ptr<CXDiagnosticSet, decltype(&clang_disposeDiagnosticSet)> self;
    // diagnostic_set(const char * file, CXLoadDiag_Error * error, CXString * error_string) : self(clang_loadDiagnostics(file, error, error_string))
    // {}
    

    diagnostic_set(CXDiagnostic s) : self(s, clang_disposeDiagnosticSet)
    {}
    struct diagnostic
    {
        detail::unique_ptr<CXDiagnostic, decltype(&clang_disposeDiagnostic)> self;
        diagnostic(CXDiagnostic s) : self(s, clang_disposeDiagnostic)
        {}
        // diagnostic(CXCodeCompleteResults * results, unsigned index) : self(clang_codeCompleteGetDiagnostic(results, index))
        // {}
        diagnostic_set get_child_diagnostics()
        {
            return clang_getChildDiagnostics(self.get());
        }
        string format_diagnostic(unsigned options)
        {
            return clang_formatDiagnostic(self.get(), options);
        }
        CXDiagnosticSeverity get_severity()
        {
            return clang_getDiagnosticSeverity(self.get());
        }
        source_location get_location()
        {
            return clang_getDiagnosticLocation(self.get());
        }
        string get_spelling()
        {
            return clang_getDiagnosticSpelling(self.get());
        }
        string get_option(CXString * disable)
        {
            return clang_getDiagnosticOption(self.get(), disable);
        }
        unsigned get_category()
        {
            return clang_getDiagnosticCategory(self.get());
        }
        string get_category_text()
        {
            return clang_getDiagnosticCategoryText(self.get());
        }
        unsigned get_num_ranges()
        {
            return clang_getDiagnosticNumRanges(self.get());
        }
        source_range get_range(unsigned range)
        {
            return clang_getDiagnosticRange(self.get(), range);
        }
        auto get_fix_its()
        {
            return detail::make_index_range(0, clang_getDiagnosticNumFixIts(self.get()), [this](unsigned i)
            {
                fix_it x;
                x.replacement = clang_getDiagnosticFixIt(self.get(), i, &x.range.self);
                return x;
            });
        }
    };

    unsigned size() const
    {
        return clang_getNumDiagnosticsInSet(self.get());
    }

    diagnostic operator()(unsigned index) const
    {
        return clang_getDiagnosticInSet(self.get(), index);
    }
    
    using iterator = detail::index_iterator<const diagnostic_set>;
    using const_iterator = detail::index_iterator<const diagnostic_set>;

    iterator begin() const
    {
        return iterator(0, *this);
    }

    iterator end() const
    {
        return iterator(size(), *this);
    }
};

struct comment
{
    CXComment self;
    comment(CXComment s) : self(s)
    {}
    CXCommentKind get_kind()
    {
        return clang_Comment_getKind(self);
    }
    auto get_children()
    {
        return detail::make_index_range(0, clang_Comment_getNumChildren(self), [this](unsigned i) 
        {
            return comment(clang_Comment_getChild(self, i));
        });
    }
    unsigned is_whitespace()
    {
        return clang_Comment_isWhitespace(self);
    }
    unsigned has_trailing_newline()
    {
        return clang_InlineContentComment_hasTrailingNewline(self);
    }
    string get_text()
    {
        return clang_TextComment_getText(self);
    }
    string get_inline_command_name()
    {
        return clang_InlineCommandComment_getCommandName(self);
    }
    CXCommentInlineCommandRenderKind get_render_kind()
    {
        return clang_InlineCommandComment_getRenderKind(self);
    }
    unsigned get_inline_num_args()
    {
        return clang_InlineCommandComment_getNumArgs(self);
    }
    string get_inline_arg_text(unsigned arg_idx)
    {
        return clang_InlineCommandComment_getArgText(self, arg_idx);
    }
    string get_tag_name()
    {
        return clang_HTMLTagComment_getTagName(self);
    }
    unsigned is_self_closing()
    {
        return clang_HTMLStartTagComment_isSelfClosing(self);
    }
    auto get_tag_attributes()
    {
        return detail::make_index_range(0, clang_HTMLStartTag_getNumAttrs(self), [this](unsigned i) 
        {
            return std::make_pair(
                string(clang_HTMLStartTag_getAttrName(self, i)), 
                string(clang_HTMLStartTag_getAttrValue(self, i))
            );
        });
    }
    string get_block_command_name()
    {
        return clang_BlockCommandComment_getCommandName(self);
    }
    auto get_block_args()
    {
        return detail::make_index_range(0, clang_BlockCommandComment_getNumArgs(self), [this](unsigned i) 
        {
            return string(clang_BlockCommandComment_getArgText(self, i));
        });
    }
    comment get_paragraph()
    {
        return clang_BlockCommandComment_getParagraph(self);
    }
    string get_param_name()
    {
        return clang_ParamCommandComment_getParamName(self);
    }
    unsigned is_param_index_valid()
    {
        return clang_ParamCommandComment_isParamIndexValid(self);
    }
    unsigned get_param_index()
    {
        return clang_ParamCommandComment_getParamIndex(self);
    }
    unsigned is_direction_explicit()
    {
        return clang_ParamCommandComment_isDirectionExplicit(self);
    }
    CXCommentParamPassDirection get_direction()
    {
        return clang_ParamCommandComment_getDirection(self);
    }
    string get_template_param_name()
    {
        return clang_TParamCommandComment_getParamName(self);
    }
    unsigned is_param_position_valid()
    {
        return clang_TParamCommandComment_isParamPositionValid(self);
    }
    unsigned get_depth()
    {
        return clang_TParamCommandComment_getDepth(self);
    }
    unsigned get_index(unsigned depth)
    {
        return clang_TParamCommandComment_getIndex(self, depth);
    }
    string get_block_text()
    {
        return clang_VerbatimBlockLineComment_getText(self);
    }
    string get_line_text()
    {
        return clang_VerbatimLineComment_getText(self);
    }
    string get_as_string()
    {
        return clang_HTMLTagComment_getAsString(self);
    }
    string get_as_html()
    {
        return clang_FullComment_getAsHTML(self);
    }
    string get_as_xml()
    {
        return clang_FullComment_getAsXML(self);
    }
};
struct type
{
    CXType self;
    type(CXType s) : self(s)
    {}
    string get_spelling()
    {
        return clang_getTypeSpelling(self);
    }
    bool equal_types(type b)
    {
        return clang_equalTypes(self, b.self);
    }
    type get_canonical_type()
    {
        return clang_getCanonicalType(self);
    }
    bool is_const_qualified_type()
    {
        return clang_isConstQualifiedType(self);
    }
    bool is_volatile_qualified_type()
    {
        return clang_isVolatileQualifiedType(self);
    }
    bool is_restrict_qualified_type()
    {
        return clang_isRestrictQualifiedType(self);
    }
    type get_pointee_type()
    {
        return clang_getPointeeType(self);
    }
    // cursor get_declaration()
    // {
    //     return clang_getTypeDeclaration(self);
    // }
    CXCallingConv get_function_calling_conv()
    {
        return clang_getFunctionTypeCallingConv(self);
    }
    type get_result_type()
    {
        return clang_getResultType(self);
    }
    auto get_arg_types()
    {
        return detail::make_index_range(0, clang_getNumArgTypes(self), [this](unsigned i) 
        {
            return type(clang_getArgType(self, i));
        });
    }
    bool is_function_variadic()
    {
        return clang_isFunctionTypeVariadic(self);
    }
    bool is_pod_type()
    {
        return clang_isPODType(self);
    }
    type get_element_type()
    {
        return clang_getElementType(self);
    }
    long long get_num_elements()
    {
        return clang_getNumElements(self);
    }
    type get_array_element_type()
    {
        return clang_getArrayElementType(self);
    }
    long long get_array_size()
    {
        return clang_getArraySize(self);
    }
    long long get_align_of()
    {
        return clang_Type_getAlignOf(self);
    }
    type get_class_type()
    {
        return clang_Type_getClassType(self);
    }
    long long get_size_of()
    {
        return clang_Type_getSizeOf(self);
    }
    long long get_offset_of(const char * s)
    {
        return clang_Type_getOffsetOf(self, s);
    }
    CXRefQualifierKind get_cxx_ref_qualifier()
    {
        return clang_Type_getCXXRefQualifier(self);
    }
};

struct completion_string
{
    CXCompletionString self;
    completion_string(CXCompletionString s) : self(s)
    {}
    CXCompletionChunkKind get_completion_chunk_kind(unsigned chunk_number)
    {
        return clang_getCompletionChunkKind(self, chunk_number);
    }
    string get_completion_chunk_text(unsigned chunk_number)
    {
        return clang_getCompletionChunkText(self, chunk_number);
    }
    completion_string get_completion_chunk_completion_string(unsigned chunk_number)
    {
        return clang_getCompletionChunkCompletionString(self, chunk_number);
    }
    unsigned get_num_completion_chunks()
    {
        return clang_getNumCompletionChunks(self);
    }
    unsigned get_completion_priority()
    {
        return clang_getCompletionPriority(self);
    }
    CXAvailabilityKind get_completion_availability()
    {
        return clang_getCompletionAvailability(self);
    }
    unsigned get_completion_num_annotations()
    {
        return clang_getCompletionNumAnnotations(self);
    }
    string get_completion_annotation(unsigned annotation_number)
    {
        return clang_getCompletionAnnotation(self, annotation_number);
    }
    string get_completion_parent(CXCursorKind * kind)
    {
        return clang_getCompletionParent(self, kind);
    }
    string get_completion_brief_comment()
    {
        return clang_getCompletionBriefComment(self);
    }
};

struct module
{
    CXModule self;
    module(CXModule s) : self(s)
    {}
    file get_ast_file()
    {
        return clang_Module_getASTFile(self);
    }
    module get_parent()
    {
        return clang_Module_getParent(self);
    }
    string get_name()
    {
        return clang_Module_getName(self);
    }
    string get_full_name()
    {
        return clang_Module_getFullName(self);
    }
};

struct cursor
{
    CXCursor self;
    cursor() : self(clang_getNullCursor())
    {}
    cursor(CXCursor s) : self(s)
    {}
    unsigned equal_cursors(cursor cursor_var)
    {
        return clang_equalCursors(self, cursor_var.self);
    }
    bool is_null()
    {
        return clang_Cursor_isNull(self);
    }
    unsigned hash()
    {
        return clang_hashCursor(self);
    }
    CXCursorKind get_kind()
    {
        return clang_getCursorKind(self);
    }
    CXLinkageKind get_linkage()
    {
        return clang_getCursorLinkage(self);
    }
    CXAvailabilityKind get_availability()
    {
        return clang_getCursorAvailability(self);
    }
    int get_platform_availability(int * always_deprecated, CXString * deprecated_message, int * always_unavailable, CXString * unavailable_message, CXPlatformAvailability * availability, int availability_size)
    {
        return clang_getCursorPlatformAvailability(self, always_deprecated, deprecated_message, always_unavailable, unavailable_message, availability, availability_size);
    }
    CXLanguageKind get_language()
    {
        return clang_getCursorLanguage(self);
    }
    module get_module()
    {
        return clang_Cursor_getModule(self);
    }
    cursor get_semantic_parent()
    {
        return clang_getCursorSemanticParent(self);
    }
    cursor get_lexical_parent()
    {
        return clang_getCursorLexicalParent(self);
    }
    void get_overridden_cursors(CXCursor ** overridden, unsigned * num_overridden)
    {
        clang_getOverriddenCursors(self, overridden, num_overridden);
    }
    file get_included_file()
    {
        return clang_getIncludedFile(self);
    }
    source_location get_location()
    {
        return clang_getCursorLocation(self);
    }
    source_range get_extent()
    {
        return clang_getCursorExtent(self);
    }
    type get_type()
    {
        return clang_getCursorType(self);
    }
    type get_typedef_decl_underlying_type()
    {
        return clang_getTypedefDeclUnderlyingType(self);
    }
    type get_enum_decl_integer_type()
    {
        return clang_getEnumDeclIntegerType(self);
    }
    long long get_enum_constant_decl_value()
    {
        return clang_getEnumConstantDeclValue(self);
    }
    unsigned long long get_enum_constant_decl_unsigned_value()
    {
        return clang_getEnumConstantDeclUnsignedValue(self);
    }
    int get_field_decl_bit_width()
    {
        return clang_getFieldDeclBitWidth(self);
    }
    auto get_arguments()
    {
        return detail::make_index_range(0, clang_Cursor_getNumArguments(self), [this](int i)
        {
            return cursor(clang_Cursor_getArgument(self, i));
        });
    }
    string get_decl_obj_c_type_encoding()
    {
        return clang_getDeclObjCTypeEncoding(self);
    }
    type get_result_type()
    {
        return clang_getCursorResultType(self);
    }
    bool is_bit_field()
    {
        return clang_Cursor_isBitField(self);
    }
    bool is_virtual_base()
    {
        return clang_isVirtualBase(self);
    }
    CX_CXXAccessSpecifier get_cxx_access_specifier()
    {
        return clang_getCXXAccessSpecifier(self);
    }
    auto get_overloaded_decls()
    {
        return detail::make_index_range(0, clang_getNumOverloadedDecls(self), [this](int i)
        {
            return cursor(clang_getOverloadedDecl(self, i));
        });
    }
    type get_ib_outlet_collection_type()
    {
        return clang_getIBOutletCollectionType(self);
    }
    template<class F>
    unsigned visit_children(F f)
    {
        CXCursorVisitor visitor = [](CXCursor c, CXCursor parent, CXClientData data) -> CXChildVisitResult
        {
            return (*reinterpret_cast<F*>(data))(cursor(c), cursor(parent));
        };
        return clang_visitChildren(self, visitor, &f);
    }
    string get_usr()
    {
        return clang_getCursorUSR(self);
    }
    string get_spelling()
    {
        return clang_getCursorSpelling(self);
    }
    source_range get_spelling_name_range(unsigned piece_index, unsigned options)
    {
        return clang_Cursor_getSpellingNameRange(self, piece_index, options);
    }
    string get_display_name()
    {
        return clang_getCursorDisplayName(self);
    }
    cursor get_referenced()
    {
        return clang_getCursorReferenced(self);
    }
    cursor get_definition()
    {
        return clang_getCursorDefinition(self);
    }
    bool is_definition()
    {
        return clang_isCursorDefinition(self);
    }
    cursor get_canonical_cursor()
    {
        return clang_getCanonicalCursor(self);
    }
    int get_obj_c_selector_index()
    {
        return clang_Cursor_getObjCSelectorIndex(self);
    }
    bool is_dynamic_call()
    {
        return clang_Cursor_isDynamicCall(self);
    }
    type get_receiver_type()
    {
        return clang_Cursor_getReceiverType(self);
    }
    unsigned get_obj_c_property_attributes(unsigned reserved)
    {
        return clang_Cursor_getObjCPropertyAttributes(self, reserved);
    }
    unsigned get_obj_c_decl_qualifiers()
    {
        return clang_Cursor_getObjCDeclQualifiers(self);
    }
    unsigned is_obj_c_optional()
    {
        return clang_Cursor_isObjCOptional(self);
    }
    bool is_variadic()
    {
        return clang_Cursor_isVariadic(self);
    }
    source_range get_comment_range()
    {
        return clang_Cursor_getCommentRange(self);
    }
    string get_raw_comment_text()
    {
        return clang_Cursor_getRawCommentText(self);
    }
    string get_brief_comment_text()
    {
        return clang_Cursor_getBriefCommentText(self);
    }
    comment get_parsed_comment()
    {
        return clang_Cursor_getParsedComment(self);
    }
    bool is_pure_virtual()
    {
        return clang_CXXMethod_isPureVirtual(self);
    }
    bool is_static()
    {
        return clang_CXXMethod_isStatic(self);
    }
    bool is_virtual()
    {
        return clang_CXXMethod_isVirtual(self);
    }
    CXCursorKind get_template_kind()
    {
        return clang_getTemplateCursorKind(self);
    }
    cursor get_specialized_template()
    {
        return clang_getSpecializedCursorTemplate(self);
    }
    source_range get_reference_name_range(unsigned name_flags, unsigned piece_index)
    {
        return clang_getCursorReferenceNameRange(self, name_flags, piece_index);
    }
    void get_definition_spelling_and_extent(const char ** start_buf, const char ** end_buf, unsigned * start_line, unsigned * start_column, unsigned * end_line, unsigned * end_column)
    {
        clang_getDefinitionSpellingAndExtent(self, start_buf, end_buf, start_line, start_column, end_line, end_column);
    }
    completion_string get_completion_string()
    {
        return clang_getCursorCompletionString(self);
    }
    template<class F>
    static CXCursorAndRangeVisitor make_range_visitor(F& f)
    {
        CXCursorAndRangeVisitor visitor = {};
        visitor.context = &f;
        visitor.visit = [](void *context, CXCursor c, CXSourceRange r) -> CXVisitorResult
        {
            return (*(reinterpret_cast<F*>(context)))(cursor(c), source_range(r));
        };
        return visitor;
    }
    template<class F>
    CXResult find_references_in_file(file file, F f)
    {
        return clang_findReferencesInFile(self, file.self, make_range_visitor(f));
    }
#ifdef __has_feature
#  if __has_feature(blocks)
    CXResult find_references_in_file_with_block(file file_var, CXCursorAndRangeVisitorBlock cursor_and_range_visitor_block_var)
    {
        return clang_findReferencesInFileWithBlock(self, file_var, cursor_and_range_visitor_block_var);
    }
#endif
#endif
};

struct code_complete_results
{
    std::unique_ptr<CXCodeCompleteResults, decltype(&clang_disposeCodeCompleteResults)> results;
    using iterator = CXCompletionResult*;
    using const_iterator = CXCompletionResult*;

    code_complete_results(CXCodeCompleteResults* r) : results(r, &clang_disposeCodeCompleteResults)
    {}

    std::size_t size() const
    {
        if (results == nullptr) return 0;
        else return results->NumResults;
    }

    iterator begin()
    {
        if (results == nullptr) return nullptr;
        else return results->Results;
    }

    iterator end()
    {
        if (results == nullptr) return nullptr;
        else return results->Results + results->NumResults;
    }
};

struct compile_command
{
    CXCompileCommand self;
    compile_command(CXCompileCommand s) : self(s)
    {}
    string get_directory()
    {
        return clang_CompileCommand_getDirectory(self);
    }
    auto get_args()
    {
        return detail::make_index_range(0, clang_CompileCommand_getNumArgs(self), [this](int i)
        {
            return clang_CompileCommand_getArg(self, i);
        });
    }
    auto get_mapped_sources()
    {
        return detail::make_index_range(0, clang_CompileCommand_getNumMappedSources(self), [this](int i)
        {
            return std::make_pair(clang_CompileCommand_getMappedSourcePath(self, i), clang_CompileCommand_getMappedSourceContent(self, i));
        });
    }
};
struct compile_commands
{
    detail::unique_ptr<CXCompileCommands, decltype(&clang_CompileCommands_dispose)> self;
    compile_commands(CXCompileCommands s) : self(s, &clang_CompileCommands_dispose)
    {}
    unsigned size() const
    {
        return clang_CompileCommands_getSize(self.get());
    }
    compile_command operator()(unsigned i) const
    {
        return clang_CompileCommands_getCommand(self.get(), i);
    }

    using iterator = detail::index_iterator<const compile_commands>;
    using const_iterator = detail::index_iterator<const compile_commands>;

    iterator begin() const
    {
        return iterator(0, *this);
    }

    iterator end() const
    {
        return iterator(size(), *this);
    }
};
struct compilation_database
{
    using self_ptr = detail::unique_ptr<CXCompilationDatabase, decltype(&clang_CompilationDatabase_dispose)>;
    self_ptr self;
    compilation_database(string_view build_dir) : self(nullptr, &clang_CompilationDatabase_dispose)
    {
        CXCompilationDatabase_Error error_code;
        self = self_ptr(clang_CompilationDatabase_fromDirectory(build_dir.c_str(), &error_code), &clang_CompilationDatabase_dispose);
        if (error_code != CXCompilationDatabase_Error::CXCompilationDatabase_NoError)
        {
            throw std::runtime_error("Database can't be loaded");
        }
    }
    compile_commands operator[](string_view complete_file_name) const
    {
        return this->get_compile_commands(complete_file_name);
    }
    compile_commands get_compile_commands(string_view complete_file_name) const
    {
        return clang_CompilationDatabase_getCompileCommands(self.get(), complete_file_name.c_str());
    }
    compile_commands get_all_compile_commands() const
    {
        return clang_CompilationDatabase_getAllCompileCommands(self.get());
    }
};

struct index_action;

struct translation_unit
{
    detail::shared_ptr<CXTranslationUnit> self;

    translation_unit(CXTranslationUnit tu) : self(tu, &clang_disposeTranslationUnit)
    {}

    static translation_unit from_cursor(cursor c)
    {
        return clang_Cursor_getTranslationUnit(c.self);
    }

    unsigned is_file_multiple_include_guarded(file file)
    {
        return clang_isFileMultipleIncludeGuarded(self.get(), file.self);
    }
    file get_file(const char * file_name)
    {
        return clang_getFile(self.get(), file_name);
    }
    source_location get_location(file file, unsigned line, unsigned column)
    {
        return clang_getLocation(self.get(), file.self, line, column);
    }
    source_location get_location_for_offset(file file, unsigned offset)
    {
        return clang_getLocationForOffset(self.get(), file.self, offset);
    }
    CXSourceRangeList* get_skipped_ranges(file file)
    {
        return clang_getSkippedRanges(self.get(), file.self);
    }
    // unsigned get_num_diagnostics()
    // {
    //     return clang_getNumDiagnostics(self.get());
    // }
    // std::shared_ptr<diagnostic> get_diagnostic(unsigned index)
    // {
    //     return clang_getDiagnostic(self.get(), index);
    // }
    diagnostic_set get_diagnostic_set()
    {
        return clang_getDiagnosticSetFromTU(self.get());
    }
    string get_translation_unit_spelling()
    {
        return clang_getTranslationUnitSpelling(self.get());
    }
    unsigned default_save_options()
    {
        return clang_defaultSaveOptions(self.get());
    }
    int save_translation_unit(const char * file_name, unsigned options)
    {
        return clang_saveTranslationUnit(self.get(), file_name, options);
    }
    unsigned default_reparse_options()
    {
        return clang_defaultReparseOptions(self.get());
    }
    int reparse_translation_unit(unsigned num_unsaved_files, CXUnsavedFile * unsaved_files, unsigned options)
    {
        return clang_reparseTranslationUnit(self.get(), num_unsaved_files, unsaved_files, options);
    }
    // tu_resource_usage get_cxtu_resource_usage()
    // {
    //     return clang_getCXTUResourceUsage(self);
    // }
    cursor get_translation_unit_cursor()
    {
        return clang_getTranslationUnitCursor(self.get());
    }
    cursor get_cursor(source_location source_location_var)
    {
        return clang_getCursor(self.get(), source_location_var.self);
    }
    unsigned get_num_top_level_headers(module module)
    {
        return clang_Module_getNumTopLevelHeaders(self.get(), module.self);
    }
    file get_top_level_header(module module, unsigned index)
    {
        return clang_Module_getTopLevelHeader(self.get(), module.self, index);
    }
    string get_token_spelling(token token_var)
    {
        return clang_getTokenSpelling(self.get(), token_var.self);
    }
    source_location get_token_location(token token_var)
    {
        return clang_getTokenLocation(self.get(), token_var.self);
    }
    source_range get_token_extent(token token_var)
    {
        return clang_getTokenExtent(self.get(), token_var.self);
    }
    void tokenize(source_range range, CXToken ** tokens, unsigned * num_tokens)
    {
        clang_tokenize(self.get(), range.self, tokens, num_tokens);
    }
    void annotate_tokens(CXToken * tokens, unsigned num_tokens, CXCursor * cursors)
    {
        clang_annotateTokens(self.get(), tokens, num_tokens, cursors);
    }
    void dispose_tokens(CXToken * tokens, unsigned num_tokens)
    {
        clang_disposeTokens(self.get(), tokens, num_tokens);
    }
    code_complete_results code_complete_at(const char * complete_filename, unsigned complete_line, unsigned complete_column, CXUnsavedFile * unsaved_files, unsigned num_unsaved_files, unsigned options)
    {
        return clang_codeCompleteAt(self.get(), complete_filename, complete_line, complete_column, unsaved_files, num_unsaved_files, options);
    }
    void get_inclusions(CXInclusionVisitor visitor, CXClientData client_data)
    {
        clang_getInclusions(self.get(), visitor, client_data);
    }
    template<class F>
    CXResult find_includes_in_file(file file, F f)
    {
        return clang_findIncludesInFile(self.get(), file.self, cursor::make_range_visitor(f));
    }
#ifdef __has_feature
#  if __has_feature(blocks)
    CXResult find_includes_in_file_with_block(file file_var, CXCursorAndRangeVisitorBlock cursor_and_range_visitor_block_var)
    {
        return clang_findIncludesInFileWithBlock(self, file_var, cursor_and_range_visitor_block_var);
    }
#endif
#endif
};


struct index
{
    detail::unique_ptr<CXIndex, decltype(&clang_disposeIndex)> self;
    index(CXIndex s) : self(s, &clang_disposeIndex)
    {}
    index(int exclude_declarations_from_pch, int display_diagnostics) : self(clang_createIndex(exclude_declarations_from_pch, display_diagnostics), &clang_disposeIndex)
    {}
    struct action
    {
        detail::unique_ptr<CXIndexAction, decltype(&clang_IndexAction_dispose)> self;
        action(CXIndexAction s) : self(s, &clang_IndexAction_dispose)
        {}
        int index_source_file(CXClientData client_data, IndexerCallbacks * index_callbacks, unsigned index_callbacks_size, unsigned index_options, const char * source_filename, const char * const * command_line_args, int num_command_line_args, CXUnsavedFile * unsaved_files, unsigned num_unsaved_files, CXTranslationUnit * out_tu, unsigned tu_options)
        {
            return clang_indexSourceFile(self.get(), client_data, index_callbacks, index_callbacks_size, index_options, source_filename, command_line_args, num_command_line_args, unsaved_files, num_unsaved_files, out_tu, tu_options);
        }
        int index_translation_unit(CXClientData client_data, IndexerCallbacks * index_callbacks, unsigned index_callbacks_size, unsigned index_options, translation_unit translation_unit_var)
        {
            return clang_indexTranslationUnit(self.get(), client_data, index_callbacks, index_callbacks_size, index_options, translation_unit_var.self.get());
        }
    };
    void set_global_options(unsigned options)
    {
        clang_CXIndex_setGlobalOptions(self.get(), options);
    }
    unsigned get_global_options()
    {
        return clang_CXIndex_getGlobalOptions(self.get());
    }
    translation_unit create_translation_unit_from_source_file(const char * source_filename, int num_clang_command_line_args, const char * const * clang_command_line_args, unsigned num_unsaved_files, CXUnsavedFile * unsaved_files)
    {
        return clang_createTranslationUnitFromSourceFile(self.get(), source_filename, num_clang_command_line_args, clang_command_line_args, num_unsaved_files, unsaved_files);
    }
    translation_unit create_translation_unit(const char * ast_filename)
    {
        return clang_createTranslationUnit(self.get(), ast_filename);
    }
    CXErrorCode create_translation_unit2(const char * ast_filename, CXTranslationUnit * out_tu)
    {
        return clang_createTranslationUnit2(self.get(), ast_filename, out_tu);
    }
    translation_unit parse_translation_unit(const char * source_filename, const char *const * command_line_args, int num_command_line_args, CXUnsavedFile * unsaved_files, unsigned num_unsaved_files, unsigned options)
    {
        return clang_parseTranslationUnit(self.get(), source_filename, command_line_args, num_command_line_args, unsaved_files, num_unsaved_files, options);
    }
    CXErrorCode parse_translation_unit2(const char * source_filename, const char *const * command_line_args, int num_command_line_args, CXUnsavedFile * unsaved_files, unsigned num_unsaved_files, unsigned options, CXTranslationUnit * out_tu)
    {
        return clang_parseTranslationUnit2(self.get(), source_filename, command_line_args, num_command_line_args, unsaved_files, num_unsaved_files, options, out_tu);
    }
    action create()
    {
        return clang_IndexAction_create(self.get());
    }
};


// struct string
// {
//     CXString self;
//     string(CXTypeKind k) : self(clang_getTypeKindSpelling(k))
//     {}
//     string(const char * class_name) : self(clang_constructUSR_ObjCClass(class_name))
//     {}
//     string(const char * class_name, const char * category_name) : self(clang_constructUSR_ObjCCategory(class_name, category_name))
//     {}
//     string(const char * protocol_name) : self(clang_constructUSR_ObjCProtocol(protocol_name))
//     {}
//     string(const char * name, string class_usr) : self(clang_constructUSR_ObjCIvar(name, class_usr))
//     {}
//     string(const char * name, unsigned is_instance_method, string class_usr) : self(clang_constructUSR_ObjCMethod(name, is_instance_method, class_usr))
//     {}
//     string(const char * property, string class_usr) : self(clang_constructUSR_ObjCProperty(property, class_usr))
//     {}
//     string(CXCursorKind kind) : self(clang_getCursorKindSpelling(kind))
//     {}
//     string(CXCodeCompleteResults * results) : self(clang_codeCompleteGetContainerUSR(results))
//     {}
//     string(CXCodeCompleteResults * results) : self(clang_codeCompleteGetObjCSelector(results))
//     {}
//     string(void void_var) : self(clang_getClangVersion(void_var))
//     {}
// };

struct idx_loc
{
    CXIdxLoc self;
    void get_file_location(CXIdxClientFile * index_file, CXFile * file, unsigned * line, unsigned * column, unsigned * offset)
    {
        clang_indexLoc_getFileLocation(self, index_file, file, line, column, offset);
    }
    source_location get_cx_source_location()
    {
        return clang_indexLoc_getCXSourceLocation(self);
    }
};
struct remapping
{
    CXRemapping self;
    remapping(const char * path) : self(clang_getRemappings(path))
    {}
    remapping(const char ** file_paths, unsigned num_files) : self(clang_getRemappingsFromFileList(file_paths, num_files))
    {}
    remapping(const remapping&)=delete;
    remapping& operator=(const remapping&)=delete;
    ~remapping()
    {
        clang_remap_dispose(self);
    }
    unsigned get_num_files()
    {
        return clang_remap_getNumFiles(self);
    }
    void get_filenames(unsigned index, CXString * original, CXString * transformed)
    {
        clang_remap_getFilenames(self, index, original, transformed);
    }
};

}

#endif