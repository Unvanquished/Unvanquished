import jinja2
import yaml

class Attribute:
    def __init__(self, name, typ):
        self.name = name
        self.typ = typ

    def get_change_enum_name(self):
        return 'ATTRIB_' + self.name.upper()

    def get_setter_name(self):
        return 'Set' + self.name[0].upper() + self.name[1:]

    def __repr__(self):
        return 'Attribute({}, {})'.format(self.name, self.typ)

class Message:
    def __init__(self, name, args):
        self.name = name
        self.args = args
        self.args_list = list(self.args.items())

    def get_enum_name(self):
        return 'MSG_' + self.name.upper()

    def get_function_args(self):
        args = []
        for arg in self.args_list:
            args.append(' '.join(arg))
        return ', '.join(args)

    def get_tuple_type(self):
        if self.args_list == []:
            return 'nullptr_t'
        return 'std::tuple<{}>'.format(', '.join(list(zip(*self.args_list))[1]))

    def get_args_names(self):
        if self.args_list == []:
            return ''
        return ', '.join(map(str, list(zip(*self.args_list))[0]))

    def __repr__(self):
        return 'Message({}, {})'.format(self.name, self.args)

class Component:
    def __init__(self, name, parameters=None, attributes=None, messages=None, dependencies=None, inherits=None):
        self.name = name
        self.params = parameters
        self.param_list = list(parameters.items())
        self.attribs = attributes
        self.messages = messages
        self.priority = None

    def gather_dependencies(self, attributes, messages, components):
        self.attribs = list(map(lambda attrib: attributes[attrib], self.attribs))

    def for_each_component_dependencies(self, fun):
        pass

    def get_type_name(self):
        return self.name + "Component"

    def get_base_type_name(self):
        return self.name + "ComponentBase"

    def get_param_declarations(self):
        return list(map(lambda p: p[1] + ' ' + p[0], self.param_list))

    def get_own_param_declarations(self):
        #TODO
        return list(map(lambda p: p[1] + ' ' + p[0], self.param_list))

    def get_param_names(self):
        return list(map(lambda p: p[0], self.param_list))

    def get_own_param_names(self):
        #TODO
        return list(map(lambda p: p[0], self.param_list))

    def get_attribs(self):
        return self.attribs

    def get_own_attribs(self):
        #TODO
        return self.attribs

    def get_attrib_declarations(self):
        return list(map(lambda a: 'const ' + a.typ + '& ' + a.name, self.attribs))

    def get_own_attrib_declarations(self):
        #TODO
        return list(map(lambda a: 'const ' + a.typ + '& ' + a.name, self.attribs))

    def get_attrib_names(self):
        return list(map(lambda a: a.name, self.attribs))

    def get_own_attrib_names(self):
        #TODO
        return list(map(lambda a: a.name, self.attribs))

    def __repr__(self):
        return "Component({}, ...)".format(self.name)

class Entity:
    def __init__(self, name, components):
        self.name = name
        self.components = components

#############################################################################

def load_attributes(definitions):
    attribs = {}
    for (name, typ) in definitions['attributes'].items():
        attribs[name] = Attribute(name, typ)
    return attribs

def load_messages(definitions):
    messages = {}
    for (name, args) in definitions['messages'].items():
        if args == None:
            args = {}
        messages[name] = Message(name, args)
    return messages

def load_components(definitions):
    components = {}
    for (name, kwargs) in definitions['components'].items():
        if not 'attributes' in kwargs:
            kwargs['attributes'] = []
        if not 'parameters' in kwargs:
            kwargs['parameters'] = {}
        components[name] = Component(name, **kwargs)
    return components

def load_entities(definitions):
    entities = {}
    for (name, components) in definitions['entities'].items():
        entities[name] = Entity(name, components)
    return entities

#############################################################################

# Not handled yet
# - Component dependencies
# - Entity definitions
# - Component parameters
# - Component initialization
# - Component inheritance

# Maybe add
# - Entity inheritance (sorta entity templates)

def topo_sort_components(components):
    sorted_components = []
    NOT_VISITED, VISITING, VISITED = range(0, 3)

    for component in components:
        component.temp_visited = NOT_VISITED

    def handle_component(component):
        if component.temp_visited == VISITING:
            raise 'The component dep graph has a cycle'
        elif component.temp_visited == NOT_VISITED:
            component.temp_visited = VISITING
            component.for_each_component_dependencies(handle_component_deps)
            component.temp_visited = VISITED
            sorted_components.append(component)

    def handle_component_deps(deps):
        for dep in deps:
            handle_component(dep)

    for component in components:
        handle_component(component)

    for component in components:
        del component.temp_visited

    return sorted_components

if __name__ == '__main__':
    f = open('def.yaml')
    definitions = yaml.load(f.read())
    f.close()

    # Load everything from the file
    attributes = load_attributes(definitions)
    attribute_list = list(attributes.values())

    messages = load_messages(definitions)
    message_list = list(messages.values())

    components = load_components(definitions)
    component_list = list(components.values())

    entities = load_entities(definitions)
    entity_list = list(entities.values())

    # Compute stuff
    sorted_components = topo_sort_components(component_list)
    for (i, component) in enumerate(sorted_components):
        component.priority = i
        component.gather_dependencies(attributes, messages, components)

    template_env = jinja2.Environment(loader=jinja2.FileSystemLoader('templates'))

    template_params = {
        'attributes': attribute_list,
        'messages': message_list,
        'components': component_list,
        'entities': entity_list
    }

    implementation_h_template = template_env.get_template('implementation.h')
    print(implementation_h_template.render(**template_params))

    print()
    print()
    print()
    print("// AND NOW FOR THE .CPP")
    print()
    print()
    print()

    implementation_cpp_template = template_env.get_template('implementation.cpp')
    print(implementation_cpp_template.render(**template_params))
