#!/usr/bin/python

import jinja2
import yaml
from collections import namedtuple
import argparse, sys
import os.path

class CommonAttribute:
    def __init__(self, name, typ):
        self.name = name
        self.typ = typ

    def get_declaration(self):
        return self.typ + " " + self.name

    def get_initializer(self):
        return self.name + "(" + self.name + ")"

    def get_name(self):
        return self.name

class Message:
    def __init__(self, name, args):
        self.name = name
        self.args = args

    def get_name(self):
        return self.name

    def get_num_args(self):
        return len(self.args)

    def get_enum_name(self):
        return 'MSG_' + self.name.upper()

    def get_handler_name(self):
            return 'Handle' + self.name

    def get_function_args(self):
        args = []
        for arg in self.args:
            args.append(arg[1] + ' ' + arg[0])
        return ', '.join(args)

    def get_handler_declaration(self):
        return self.get_handler_name() + '(' + self.get_function_args() + ')'

    def get_return_type(self):
        return 'void';

    def get_tuple_type(self):
        if self.args == []:
            return 'std::nullptr_t'
        return 'std::tuple<{}>'.format(', '.join(list(zip(*self.args))[1]))

    def get_arg_names(self):
        return [arg[0] for arg in self.args]

    # TODO: Rename to something that doesn't clash as much with get_arg_names
    def get_args_names(self):
        if self.args == []:
            return ''
        return ', '.join(map(str, list(zip(*self.args))[0]))

    def get_unpacked_tuple_args(self, tuple_name):
        return ', '.join(['std::get<' + str(i) + '>(' + tuple_name +')' for i in range(len(self.args))])

    def get_arg_number(self):
        return len(self.args)

    def is_attrib(self):
        return self.attrib != None

    def get_attrib(self):
        return self.attrib

    def __repr__(self):
        return 'Message({}, {})'.format(self.name, self.args)

class Component:
    def __init__(self, name, parameters=None, messages=None, requires=None, inherits=None):
        self.name = name
        self.params = parameters
        self.param_list = list(parameters.items())
        self.messages = messages
        self.priority = None
        self.requires = requires
        self.inherits = inherits

    def gather_dependencies(self, messages, components):
        self.messages = list(map(lambda message: messages[message], self.messages))

    def gather_component_dependencies(self, components):
        self.requires = list(map(lambda component: components[component], self.requires))
        self.inherits = list(map(lambda component: components[component], self.inherits))

    def for_each_component_dependencies(self, fun):
        for require in self.requires:
            fun(require)
        for inherit in self.inherits:
            fun(inherit)

    def get_name(self):
        return self.name

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

    def get_constructor_declaration(self):
        return self.get_type_name() + '(Entity *entity' + ', '.join([''] + self.get_param_declarations()) + ')'

    def get_super_call(self):
        return self.get_base_type_name() + '(entity' + ', '.join([''] + list(map(lambda p: p[0], self.param_list))) + ')'

    def get_param_names(self):
        return list(map(lambda p: p[0], self.param_list))

    def get_own_param_names(self):
        #TODO
        return list(map(lambda p: p[0], self.param_list))

    def get_required_components(self):
        return self.requires

    def get_own_required_components(self):
        #TODO
        return self.requires

    def get_required_component_declarations(self):
        return list(map(lambda c: c.get_type_name() + '& r_' + c.get_type_name(), self.requires))

    def get_own_required_component_declarations(self):
        #TODO
        return list(map(lambda c: c.get_type_name() + '& r_' + c.get_type_name(), self.requires))

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
        for component in self.components:
            self.messages |= set(component.get_messages_to_handle())
        self.messages = list(self.messages)

    def get_type_name(self):
        return self.name + "Entity"

    def get_components(self):
        return self.components

    def get_params(self):
        return self.params

    def get_messages_to_handle(self):
        return self.messages

    def get_message_handler_name(self, message):
        return "h_" + self.name + "_" + message.name

#############################################################################

def load_general(definitions):
    defs = definitions['general']
    common_entity_attributes = []
    for attrib in defs['common_entity_attributes']:
        common_entity_attributes.append(CommonAttribute(attrib['name'], attrib['type']))
    return namedtuple('general', 'common_entity_attributes')(common_entity_attributes)

def load_messages(definitions):
    messages = {}
    for (name, args) in definitions['messages'].items():
        if args == None:
            args = []
        messages[name] = Message(name, [(arg['name'], arg['type']) for arg in args])
    return messages

def load_components(definitions):
    components = {}
    for (name, kwargs) in definitions['components'].items():
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
        if not 'components' in kwargs or kwargs['components'] == None:
            kwargs['components'] = {}
        entities[name] = Entity(name, kwargs['components'])
    return entities

############################################################################

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

def my_filter(text):
    lines = []
    for line in text.split('\n'):
        # Remove all blank lines
        if line.strip() == '':
            continue
        # Remove the "template" comments
        if line.strip().startswith("//*"):
            continue
        # Handle the command comments
        if line.strip().startswith("//%"):
            #look at the characters stream after the //%
            for char in line.strip()[3:]:
                if char == 'L':
                    lines.append('')
            continue

        lines.append(line)
    return '\n'.join(lines)

def my_open_read(filename):
    if filename == "-":
        return sys.stdin
    else:
        return open(filename, "r")

def my_open_write(filename):
    if filename == "-":
        return sys.stdout
    else:
        return open(filename, "w")

def render(input_file, output_file, template_params):
    template = template_env.get_template(input_file)
    output_file.write(my_filter(template.render(**template_params)))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Outputs C++ plumbing code for the gamelogic given a component definitions.")
    parser.add_argument('definitions', metavar='DEFS', nargs=1, type=my_open_read, help ="The definitions to use, - for stdin.")
    parser.add_argument('-o', '--output-dir', nargs=1, default=None, metavar='DIR', type=str, help="Output directory for the generated source files.")
    parser.add_argument('--no-include-helper', dest='include_helper', const=False, default=True, action='store_const', help="Skip the generation of the include helpers")

    args = parser.parse_args()

    # Load everything from the file
    definitions = yaml.load(args.definitions[0])
    args.definitions[0].close()

    general = load_general(definitions)

    messages = load_messages(definitions)
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
        component.gather_dependencies(messages, components)

    for entity in entity_list:
        entity.gather_components(components)

    template_env = jinja2.Environment(loader=jinja2.FileSystemLoader('templates'))

    infiles = {
        'declaration':    'Components.h',
        'implementation': 'Components.cpp',
        'entities':       'ComponentsEntities.h',
        'includehelper':  'ComponentImplementationInclude.h',
        'skeleton_cpp':   'Component.cpp',
        'skeleton_h':     'Component.h'
    }

    outfiles = {
        'declaration':    'Components.h',
        'implementation': 'Components.cpp',
        'entities':       'ComponentsEntities.h',
        'includehelper':  'ComponentImplementationInclude.h'
    }

    outdirs = {
        'components': 'components',
        'skeletons':  'skel'
    }

    template_params = {
        'general': general,
        'messages': message_list,
        'components': component_list,
        'entities': entity_list,
        'enumerate': enumerate,
        'files': outfiles,
        'dirs': outdirs
    }

    if args.output_dir != None:
        outdir = args.output_dir[0] + os.path.sep

        with open(outdir + outfiles['declaration'], "w") as outfile:
            render(infiles['declaration'], outfile, template_params)

        with open(outdir + outfiles['implementation'], "w") as outfile:
            render(infiles['implementation'], outfile, template_params)

        with open(outdir + outfiles['entities'], "w") as outfile:
            render(infiles['entities'], outfile, template_params)

        if args.include_helper:
            with open(outdir + outfiles['includehelper'], "w") as outfile:
                render(infiles['includehelper'], outfile, template_params)

        outdir += outdirs['components'] + os.path.sep

        if not os.path.isdir(outdir):
            os.mkdir(outdir)

        outdir += outdirs['skeletons'] + os.path.sep

        if not os.path.isdir(outdir):
            os.mkdir(outdir)

        # TODO: Copy skeletons to target if that doesn't exist
        for component in component_list:
            template_params['component'] = component

            with open(outdir + component.get_type_name() + ".cpp", "w") as outfile:
                render(infiles['skeleton_cpp'], outfile, template_params)

            with open(outdir + component.get_type_name() + ".h", "w") as outfile:
                render(infiles['skeleton_h'], outfile, template_params)

# vi:ts=4:et:ai
