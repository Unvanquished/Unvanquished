#!/usr/bin/python

import jinja2
import yaml

class Attribute:
    def __init__(self, name, typ):
        self.name = name
        self.typ = typ

    def get_setter_name(self):
        return 'Set' + self.name[0].upper() + self.name[1:]

    def get_variable_name(self):
        return 'a_' + self.name

    def get_message(self):
        return self.message

    def set_message(self, message):
        self.message = message

    def __repr__(self):
        return 'Attribute({}, {})'.format(self.name, self.typ)

class Message:
    def __init__(self, name, args, attrib = None):
        self.name = name
        self.args = args
        self.args_list = list(self.args.items())
        self.attrib = attrib

    def get_num_args(self):
        return len(self.args_list)

    def get_enum_name(self):
        if self.attrib == None:
            return 'MSG_' + self.name.upper()
        else:
            return 'ATTRIB_' + self.name.upper()

    def get_handler_name(self):
        if self.attrib == None:
            return 'On' + self.name
        else:
            return 'Attrib' + self.name

    def get_function_args(self):
        args = []
        for arg in self.args_list:
            args.append(arg[1] + ' ' + arg[0])
        return ', '.join(args)

    def get_tuple_type(self):
        if self.args_list == []:
            return 'std::nullptr_t'
        return 'std::tuple<{}>'.format(', '.join(list(zip(*self.args_list))[1]))

    def get_args_names(self):
        if self.args_list == []:
            return ''
        return ', '.join(map(str, list(zip(*self.args_list))[0]))

    def get_unpacked_tuple_args(self, tuple_name):
        return ', '.join(['std::get<' + str(i) + '>(' + tuple_name +')' for i in range(len(self.args_list))])

    def get_arg_number(self):
        return len(self.args_list)

    def is_attrib(self):
        return self.attrib != None

    def get_attrib(self):
        return self.attrib

    def __repr__(self):
        return 'Message({}, {})'.format(self.name, self.args)

class Component:
    def __init__(self, name, parameters=None, attributes=None, messages=None, requires=None, inherits=None):
        self.name = name
        self.params = parameters
        self.param_list = list(parameters.items())
        self.attribs = attributes
        self.messages = messages
        self.priority = None
        self.requires = requires
        self.inherits = inherits

    def gather_dependencies(self, attributes, messages, components):
        self.attribs = list(map(lambda attrib: attributes[attrib], self.attribs))
        self.messages = list(map(lambda message: messages[message], self.messages))
        for attrib in self.attribs:
            self.messages.append(attrib.get_message())

    def gather_component_dependencies(self, components):
        self.requires = list(map(lambda component: components[component], self.requires))
        self.inherits = list(map(lambda component: components[component], self.inherits))

    def for_each_component_dependencies(self, fun):
        for require in self.requires:
            fun(require)
        for inherit in self.inherits:
            fun(inherit)

    def get_type_name(self):
        return self.name + "Component"

    def get_base_type_name(self):
        return self.name + "ComponentBase"

    def get_variable_name(self):
        return "c_" + self.get_type_name()

    def get_priority(self):
        return self.priority

    def get_messages_to_handle(self):
        return self.messages

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

    def get_required_components(self):
        return self.requires

    def get_own_required_components(self):
        #TODO
        return self.requires

    def get_required_component_declarations(self):
        return list(map(lambda c: c.get_type_name() + '* const r_' + c.get_type_name(), self.requires))

    def get_own_required_component_declarations(self):
        #TODO
        return list(map(lambda c: c.get_type_name() + '* const r_' + c.get_type_name(), self.requires))

    def get_required_component_names(self):
        return list(map(lambda c: 'r_' + c.get_type_name(), self.requires))

    def get_own_required_component_names(self):
        #TODO
        return list(map(lambda c: 'r_' + c.get_type_name(), self.requires))

    def __repr__(self):
        return "Component({}, ...)".format(self.name)

class Entity:
    def __init__(self, name, params):
        self.name = name
        self.params = params

    def gather_components(self, components):
        self.components = list(map(lambda component: components[component], self.params.keys()))
        self.components = sorted(self.components, key = lambda component: component.priority)
        self.messages = set()
        self.attributes = set()
        for component in self.components:
            self.messages |= set(component.get_messages_to_handle())
            self.attributes |= set(component.get_attribs())
        self.messages = list(self.messages)
        self.attributes = list(self.attributes)

    def get_type_name(self):
        return self.name + "Entity"

    def get_components(self):
        return self.components

    def get_attributes(self):
        return self.attributes

    def get_params(self):
        return self.params

    def get_messages_to_handle(self):
        return self.messages

    def get_message_handler_name(self, message):
        return "h_" + self.name + "_" + message.name

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
        if not 'messages' in kwargs:
            kwargs['messages'] = []
        if not 'parameters' in kwargs:
            kwargs['parameters'] = {}
        if not 'requires' in kwargs:
            kwargs['requires'] = []
        if not 'inherits' in kwargs:
            kwargs['inherits'] = {}
        components[name] = Component(name, **kwargs)
    return components

def load_entities(definitions):
    entities = {}
    for (name, kwargs) in definitions['entities'].items():
        entities[name] = Entity(name, kwargs['components'])
    return entities

#############################################################################

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
            component.for_each_component_dependencies(handle_component)
            component.temp_visited = VISITED
            sorted_components.append(component)

    for component in components:
        handle_component(component)

    for component in components:
        del component.temp_visited

    return sorted_components

def my_print(text):
    print('\n'.join(filter(lambda line: line.strip() != '', text.split('\n'))))

if __name__ == '__main__':
    f = open('def.yaml')
    definitions = yaml.load(f.read())
    f.close()

    # Load everything from the file
    attributes = load_attributes(definitions)
    attribute_list = list(attributes.values())

    messages = load_messages(definitions)
    for attribute in attribute_list:
        message = Message(attribute.name, {"value": attribute.typ}, attribute)
        messages["attr_" + attribute.name] = message
        attribute.set_message(message)
    message_list = list(messages.values())

    components = load_components(definitions)
    component_list = list(components.values())

    entities = load_entities(definitions)
    entity_list = list(entities.values())

    # Compute stuff
    for component in component_list:
        component.gather_component_dependencies(components)

    sorted_components = topo_sort_components(component_list)
    for (i, component) in enumerate(sorted_components):
        component.priority = i
        component.gather_dependencies(attributes, messages, components)

    for entity in entity_list:
        entity.gather_components(components)

    template_env = jinja2.Environment(loader=jinja2.FileSystemLoader('templates'))

    template_params = {
        'attributes': attribute_list,
        'messages': message_list,
        'components': component_list,
        'entities': entity_list
    }

    implementation_h_template = template_env.get_template('implementation.h')
    my_print(implementation_h_template.render(**template_params))

    print()
    print()
    print()
    print("// AND NOW FOR THE .CPP")
    print()
    print()
    print()

    implementation_cpp_template = template_env.get_template('implementation.cpp')
    my_print(implementation_cpp_template.render(**template_params))
