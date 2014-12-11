import sys, re

fname = sys.argv[1]

decls = []

class_map = {}

def to_snake_case(name):
    s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1).lower()

def parse_params(params):
    result = []
    for param in params:
        p = param.strip()
        last = re.split('\W', p)[-1]
        first = p[0:-len(last)].strip()
        if len(first) > 0:
            result.append({'name': last, 'type':first})
        else:
            name = to_snake_case(last.replace('CX', '')) + '_var'
            result.append({'name': name, 'type':last})
    return result


def parse_decl(s):
    open_paren = s.find('(')
    close_paren = s.find(')')
    params = parse_params(s[open_paren+1:close_paren].split(','))
    words = s[0:open_paren].split()
    name = words[-1]
    return_type = ' '.join(words[0:-1])
    return {'params': params, 'name': name, 'return': return_type}

def map_type(type):
    if type in class_map: return class_map[type].type_name()
    else: return type.replace('struct ', '').replace('enum ', '')

def map_name(name):
    r = to_snake_case(name)
    if r in class_map: return r + '_var'
    else: return r

def gen_params(items, transform):
    return '(' + ', '.join([(transform(map_type(x['type']), map_name(x['name']))) for x in items]) + ')'

def forward_param(type, name):
    if type.startswith('std::shared_ptr'): return name + '.get()'
    else: return name

def generate_fun_params(decl):
    return gen_params(decl['params'], lambda x, y: x + ' ' + y)

def generate_forward_params(decl):
    return decl['name'] + gen_params(decl['params'], forward_param)

def generate_fun_self_params(decl):
    return gen_params(decl['params'][1:], lambda x, y: x + ' ' + y)

def generate_forward_self_params(decl):
    return decl['name'] + gen_params([{'name': 'self', 'type': 'Self'}] + decl['params'][1:], forward_param)

class CppClass:
    def __init__(self, type, name=None):
        self.name = name
        self.type = type
        if self.name == None: self.name = to_snake_case(self.type.replace('CX', ''))
        self.functions = []
        self.constructors = []
        self.destructor = None

    def add_constructor(self, c):
        self.constructors.append(c)

    def set_destructor(self, d):
        self.destructor = d

    def has_destructor(self):
        return self.destructor != None

    def type_name(self):
        if self.has_destructor(): return 'std::shared_ptr<' + self.name + '>'
        else: return self.name

    def add_function(self, f):
        self.functions.append(f)

    def generate(self):
        if len(self.functions) == 0 and len(self.constructors) == 0: return ''
        result = [self.type + ' self;']
        for c in self.constructors:
            result.append(self.name + generate_fun_params(c) + ' : self(' + generate_forward_params(c) + ')')
            result.append('{}')

        if self.destructor != None:
            result.append(self.name + '(const ' + self.name + '&)=delete;')
            result.append(self.name + '& operator=(const ' + self.name + '&)=delete;')
            result.append('~' + self.name + '()')
            result.append('{')
            result.append('    ' + self.destructor['name'] + '(self);')
            result.append('}')

        for f in self.functions:
            name = f['name'].split('_')[-1]
            return_type = map_type(f['return'])
            result.append(return_type + ' ' + to_snake_case(name) + generate_fun_self_params(f))
            result.append('{')
            return_statement = ''
            if return_type != 'void': return_statement = 'return '
            result.append('    ' + return_statement + generate_forward_self_params(f) + ';')
            result.append('}')
        return '\n'.join(['struct ' + self.name, '{'] + [('    ' + x) for x in result] + ['};'])


with open(fname) as f:
    current_decl = ''
    for line in f.readlines():
        l = line.strip()
        if l.startswith('CINDEX_LINKAGE') or len(current_decl) > 0:
            current_decl = current_decl + ' ' + l
        if l.endswith(';'): 
            if len(current_decl) > 0: decls.append(current_decl[len('CINDEX_LINKAGE')+1:].strip())
            current_decl = ''

classes = [
    CppClass('CXIndex'), 
    CppClass('CXTranslationUnit'), 
    CppClass('CXFile'),
    CppClass('CXIndexAction'),
    CppClass('CXSourceLocation'),
    CppClass('CXDiagnostic'),
    CppClass('CXDiagnosticSet'),
    CppClass('CXString'),
    CppClass('CXTUResourceUsage'),
    CppClass('CXCursor'),
    CppClass('CXSourceRange'),
    CppClass('CXCodeCompleteResults'),
    CppClass('CXUnsavedFile'),
    CppClass('CXResult'),
    CppClass('CXComment'),
    CppClass('CXType'),
    CppClass('CXCompletionString'),
    CppClass('CXIdxIncludedFileInfo'),
    CppClass('CXIdxEntityInfo'),
    CppClass('CXIdxEntityRefKind'),
    CppClass('CXIdxLoc'),
    CppClass('CXIdxClientFile'),
    CppClass('CXRemapping'),
    CppClass('CXCompilationDatabase'),
    CppClass('CXCompileCommands'),
    CppClass('CXCompileCommand'),
    CppClass('CXDiagnosticSeverity'),
    CppClass('CXToken'),
    CppClass('CXModule')
]

for c in classes:
    class_map[c.type] = c

for decl in decls:
    # print(decl)
    f = parse_decl(decl)
    type = ''
    if len(f['params']) > 0: type = f['params'][0]['type']
    if type in class_map:
        c = class_map[type]
        if len(f['params']) == 1 and 'dispose' in f['name'].lower():
            c.set_destructor(f)
        else:
            c.add_function(f)
    elif f['return'] in class_map:
        c = class_map[f['return']]
        c.add_constructor(f)

for c in classes:
    print(c.generate())
    