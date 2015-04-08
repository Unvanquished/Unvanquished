#!/usr/bin/python3

import jinja2
import yaml
from collections import namedtuple
import argparse, sys
import os.path
import re

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
        return ', '.join([arg[1] + ' ' + arg[0] for arg in self.args])

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
        return ', '.join(self.get_arg_names())

    def get_unpacked_tuple_args(self, tuple_name):
        return ', '.join(['std::get<' + str(i) + '>(' + tuple_name +')' for i in range(len(self.args))])

    def __repr__(self):
        return 'Message({}, {})'.format(self.name, self.args)

Parameter = namedtuple('Parameter', ['name', 'typ', 'default'])

class Component:
    def __init__(self, name, parameters=None, messages=None, requires=None, inherits=None, defaults=None):
        self.name = name
        self.param_list = []
        self.parameters = {}

        # Create the list of parameters, with potential default parameters
        for (name, typ) in parameters.items():
            p = None
            if name in defaults:
                p = Parameter(name, typ, defaults[name])
            else:
                p = Parameter(name, typ, None)
            self.param_list.append(p)
            self.parameters[name] = p

        self.param_list = sorted(self.param_list, key = lambda param: param.name)
        self.messages = messages
        self.priority = None
        self.requires = requires
        self.inherits = inherits

    def gather_messages(self, messages):
        self.messages = [messages[m] for m in self.messages]

    def gather_component_dependencies(self, components):
        self.requires = [components[c] for c in self.requires]
        self.inherits = [components[c] for c in self.inherits]

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
        return [p.typ + ' ' + p.name for p in self.param_list]

    def get_own_param_declarations(self):
        #TODO
        return [p.typ + ' ' + p.name for p in self.param_list]

    def get_param_names(self):
        return [p.name for p in self.param_list]

    def get_own_param_names(self):
        #TODO
        return [p.name for p in self.param_list]

    def get_required_components(self):
        return self.requires

    def get_own_required_components(self):
        #TODO
        return self.requires

    def get_required_component_declarations(self):
        return [c.get_type_name() + '& r_' + c.get_type_name() for c in self.requires]

    def get_own_required_component_declarations(self):
        #TODO
        return [c.get_type_name() + '& r_' + c.get_type_name() for c in self.requires]

    def get_required_component_names(self):
        return ['r_' + c.get_type_name() for c in self.requires]

    def get_own_required_component_names(self):
        #TODO
        return ['r_' + c.get_type_name() for c in self.requires]

    def __repr__(self):
        return "Component({}, ...)".format(self.name)

class Entity:
    def __init__(self, name, params):
        self.name = name
        self.params = params

    def gather_components(self, components):
        self.components = [components[c] for c in self.params.keys()]

        # Add dependency components.
        # TODO more efficient algorithm? (this is worst case O(nComponents)^3)
        gathering_dependencies = True
        while gathering_dependencies:
            gathering_dependencies = False
            for component in self.components:
                for dependency in component.get_required_components():
                    if dependency not in self.components:
                        self.components.append(dependency)
                        gathering_dependencies = True

        self.components = sorted(self.components, key = lambda component: component.priority)

        # Gather the list of all messages this entity can receive
        self.messages = set()
        for component in self.components:
            self.messages |= set(component.get_messages_to_handle())
        self.messages = list(self.messages)

        # Initialize the parameter state
        self.user_params = {}
        self.has_user_params = False
        for component in self.components:
            self.user_params[component.name] = {}
            if not component.name in self.params:
                self.params[component.name] = {}

        # Choose the value for each component parameter (unless it is a user defined param)
        for component in self.components:
            for param in component.param_list:
                if not param.name in self.params[component.name]:
                    if param.default != None:
                        self.params[component.name][param.name] = param.default
                    else:
                        self.user_params[component.name][param.name] = param
                        self.has_user_params = True
                elif self.params[component.name][param.name] == 'USER':
                    self.user_params[component.name][param.name] = param
                    self.has_user_params = True


    def get_type_name(self):
        return self.name + "Entity"

    def get_components(self):
        return self.components

    def get_params(self):
        return self.params

    def get_user_params(self):
        return self.user_params

    def get_has_user_params(self):
        return self.has_user_params

    def get_messages_to_handle(self):
        return self.messages

    def get_message_handler_name(self, message):
        return "h_" + self.name + "_" + message.name

#############################################################################

def convert_params(params):
    # Convert defaults to strings that C++ understands.
    for param, value in params.items():
        if type(value) == bool:
            value = str(value).lower()
        elif type(value) == str:
            # Python doesn't distinguish char and string, so we default to
            # string. If someone wants to use a char parameter, they will still
            # be able to write the value as a raw number or convert a char via
            #     !!python/object/apply:builtins.ord ['x']
            # where x is the char to be converted.
            value = '"' + value + '"'
        else:
            value = str(value)
        params[param] = value


def load_general(definitions):
    common_entity_attributes = []
    for attrib in definitions['common_entity_attributes']:
        common_entity_attributes.append(CommonAttribute(attrib['name'], attrib['type']))
    return namedtuple('general', 'common_entity_attributes')(common_entity_attributes)

def load_messages(definitions):
    messages = {}
    for (name, args) in definitions.items():
        if args == None:
            args = []
        messages[name] = Message(name, [(arg['name'], arg['type']) for arg in args])
    return messages

def load_components(definitions):
    components = {}
    for (name, kwargs) in definitions.items():
        if not 'messages' in kwargs:
            kwargs['messages'] = []

        if not 'parameters' in kwargs:
            kwargs['parameters'] = {}

        if not 'defaults' in kwargs:
            kwargs['defaults'] = {}

        if not 'requires' in kwargs:
            kwargs['requires'] = []

        if not 'inherits' in kwargs:
            kwargs['inherits'] = {}
        else:
            raise Exception("inherits not handled for now")

        convert_params(kwargs['defaults'])
        components[name] = Component(name, **kwargs)
    return components

def load_entities(definitions):
    entities = {}
    for (name, kwargs) in definitions.items():
        if not 'components' in kwargs or kwargs['components'] == None:
            kwargs['components'] = {}

        for component_name, component_params in kwargs['components'].items():
            if component_params != None:
                convert_params(component_params)

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

def parse_definitions(definitions):
    # Do the basic loading of objects, independently of other objects
    general = load_general(definitions['general'])

    messages = load_messages(definitions['messages'])
    message_list = list(messages.values())

    components = load_components(definitions['components'])
    component_list = list(components.values())

    entities = load_entities(definitions['entities'])
    entity_list = list(entities.values())

    # Link objects together

    # First make each component get its dependencies and then compute their topological order
    for component in component_list:
        component.gather_component_dependencies(components)

    sorted_components = topo_sort_components(component_list)
    for (i, component) in enumerate(sorted_components):
        component.priority = i

    for component in sorted_components:
        component.gather_messages(messages)

    for entity in entity_list:
        entity.gather_components(components)

    definitions = {
        'general': general,
        'messages': message_list,
        'components': sorted_components,
        'entities': entity_list,
    }

    return definitions

############################################################################

# A custom Jinja2 template loader that removes the extra indentation
# of the template blocks so that the output is correctly indented
class PreprocessingLoader(jinja2.BaseLoader):
    def __init__(self, path):
        self.path = path

    def get_source(self, environment, template):
        path = os.path.join(self.path, template)
        if not os.path.exists(path):
            raise jinja2.TemplateNotFound(template)
        mtime = os.path.getmtime(path)
        with open(path) as f:
            source = self.preprocess(f.read())
        return source, path, lambda: mtime == os.path.getmtime(path)

    def preprocess(self, source):
        # Remove the trailing newline
        lines = source.split("\n")

        # Compute the current indentation level of the template blocks and remove their indentation
        result = []
        indentation_level = 0
        for line in lines:
            if self.blockstart.match(line) != None:
                indentation_level += 1
            elif self.blockend.match(line) != None:
                indentation_level -=1

            result.append(self.remove_indentation(line, indentation_level))

        return self.my_filter('\n'.join(result))

    def remove_indentation(self, line, n):
        for _ in range(n):
            if line.startswith(' '):
                line = line[4:]
            elif line.startswith('\t'):
                line = line[1:]
        return line

    blockstart = re.compile('^\s*{%-?\s*(if|for).*$')
    blockend = re.compile('^\s*{%-?\s*end(if|for).*$')

    def my_filter(self, text):
        lines = []
        for line in text.split('\n'):
            # Remove the "template" comments
            if line.strip().startswith("//*"):
                continue

            lines.append(line)
        return '\n'.join(lines)

if __name__ == '__main__':
    # Command line args
    parser = argparse.ArgumentParser(
        description="Outputs C++ plumbing code for the gamelogic given a component/entity definition file.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument('definitions', metavar='DEFINITION_FILE', nargs=1, type=open, help ="The definitions to use, - for stdin.")
    parser.add_argument('-t', '--template-dir', default="templates", type=str, help="Directory with template files.")
    parser.add_argument('-o', '--output-dir', default=None, type=str, help="Output directory for the generated source files.")
    parser.add_argument('-c', '--copy-skel', action="store_true", help="Copy skeleton files in place if the file doesn't exist.")

    args = parser.parse_args()

    # Load everything from the definition file
    definitions = parse_definitions(yaml.load(args.definitions[0]))
    args.definitions[0].close()

    unique_files = {
        'backend':      'Backend.h',
        'backend_cpp':  'Backend.cpp',
        'components':   'Components.h',
        'entities':     'Entities.h'
    }

    # The output files for the unique files are prefixed with CBSE
    outfiles = dict([(key, 'CBSE' + val) for (key, val) in unique_files.items()])
    outdirs = {
        'components': 'components',
        'skeletons':  'skel'
    }

    # Create the template parameters from the definitions
    #TODO get rid of outfiles and outdirs?
    template_params = [definitions, {
        'files': outfiles,
        'dirs': outdirs,
    }]

    if args.output_dir != None:
        outdir = args.output_dir + os.path.sep
        compdir = outdir + outdirs['components'] + os.path.sep
        skeldir = compdir + outdirs['skeletons'] + os.path.sep

        # Generate a list of files to create, params_dicts will get squashed to create the template parameters
        FileToRender = namedtuple('FileToRender', ['template', 'output', 'params_dicts', 'overwrite'])
        to_render = []

        # Add unique files
        for (key, output) in outfiles.items():
            template = unique_files[key]
            to_render.append(FileToRender(template, outdir + output, template_params, True))

        # Add skeleton files
        for component in definitions['components']:
            params = template_params + [{'component': component}]
            basename = skeldir + component.get_type_name()

            to_render.append(FileToRender('Component.h', basename + '.h', params, True))
            to_render.append(FileToRender('Component.cpp', basename + '.cpp', params, True))

            if args.copy_skel:
                basename = compdir + component.get_type_name()
                to_render.append(FileToRender('Component.h', basename + '.h', params, False))
                to_render.append(FileToRender('Component.cpp', basename + '.cpp', params, False))

        # Now render the files
        env = jinja2.Environment(loader=PreprocessingLoader(args.template_dir), trim_blocks=True, lstrip_blocks=True)
        env.globals["enumerate"] = enumerate

        for render in to_render:
            if not render.overwrite and os.path.exists(render.output):
                continue

            params = {}
            for param_dict in render.params_dicts:
                params.update(param_dict)
            content = env.get_template(render.template).render(**params) + "\n"

            directory = os.path.dirname(render.output)
            if not os.path.exists(directory):
                os.makedirs(directory)

            with open(render.output, 'w') as outfile:
                outfile.write(content)

            if not render.overwrite:
                print('Added file, ' + render.output)

# vi:ts=4:et:ai
