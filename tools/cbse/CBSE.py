#!/usr/bin/env python

# Daemon CBSE Source Code
# Copyright (c) 2014-2015, Daemon Developers
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# * Neither the name of Daemon CBSE nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


import jinja2
import yaml
from collections import namedtuple, OrderedDict
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
        self.parameters = OrderedDict()

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

        # First level of required components
        self.requiredComponents = requires.keys() # Only names for now.
        self.requiredParameters = requires
        self.inherits = inherits

        # List of required components of level n >= 2, contains pairs in
        # the form dependency => firstLevel so that firstLevel is on a path
        # from this to dependency
        self.furtherDependencies = OrderedDict()

    def gather_messages(self, messages):
        self.messages = [messages[m] for m in self.messages]

    def gather_component_dependencies(self, components):
        self.requiredComponents = [components[c] for c in self.requiredComponents]
        self.inherits           = [components[c] for c in self.inherits]

    def gather_full_dependencies(self):
        registered = set(self.requiredComponents)

        for firstLevel in self.requiredComponents:
            for dependency in firstLevel.get_full_dependencies():
                if dependency in registered:
                    continue
                registered.add(dependency)
                self.furtherDependencies[dependency] = firstLevel

    def get_full_dependencies(self):
        return self.requiredComponents + list(self.furtherDependencies.keys())

    def for_each_component_dependencies(self, fun):
        for require in self.requiredComponents:
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
        return self.requiredComponents

    def get_required_parameters(self):
        return self.requiredParameters

    def get_own_required_components(self):
        #TODO
        return self.requiredComponents

    def get_required_component_declarations(self):
        return [c.get_type_name() + '& r_' + c.get_type_name() for c in self.requiredComponents]

    def get_own_required_component_declarations(self):
        #TODO
        return [c.get_type_name() + '& r_' + c.get_type_name() for c in self.requiredComponents]

    def get_required_component_names(self):
        return ['r_' + c.get_type_name() for c in self.requiredComponents]

    def get_own_required_component_names(self):
        #TODO
        return ['r_' + c.get_type_name() for c in self.requiredComponents]

    def get_own_further_dependencies(self):
        return self.furtherDependencies


    def __repr__(self):
        return "Component({}, ...)".format(self.name)

class Entity:
    def __init__(self, name, params):
        self.name = name
        self.params = params

    def gather_components(self, components, mandatory):
        self.components = [components[c] for c in mandatory] + \
                          [components[c] for c in self.params.keys() if c not in mandatory]

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
        self.user_params = OrderedDict()
        self.has_user_params = False
        for component in self.components:
            self.user_params[component.name] = OrderedDict()
            if not component.name in self.params:
                self.params[component.name] = OrderedDict()

        # Choose the value for each component parameter (unless it is a user defined param)
        for component in self.components:
            for param in component.param_list:
                if not param.name in self.params[component.name]:
                    # Use the parameters own default value, if it has one, as fallback
                    self.params[component.name][param.name] = param.default

                    # Look for a value given by another component that depends on the current one
                    dependency_sets_parameter = False
                    for dependent in self.components:
                        required_parameters = dependent.get_required_parameters()
                        if component.name not in required_parameters:
                            continue
                        if required_parameters[component.name] is None:
                            continue
                        for required_param, required_value in required_parameters[component.name].items():
                            if param.name != required_param:
                                continue
                            if dependency_sets_parameter:
                                raise Exception("Multiple components set a default value for the same parameter of a required component.")
                            self.params[component.name][param.name] = required_value
                            dependency_sets_parameter = True

                # If we still have not found a value, let the user decide
                if not param.name in self.params[component.name]:
                    self.params[component.name][param.name] = None

                # Let the user decide on the value
                if self.params[component.name][param.name] == None:
                    self.params[component.name].pop(param.name)
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
        elif value is None:
            pass
        else:
            value = str(value)
        params[param] = value


def load_general(definitions):
    if definitions is None:
        definitions = {}

    common_entity_attributes = []
    mandatory_components = []

    if 'mandatory_components' in definitions:
        mandatory_components += definitions['mandatory_components']

    if 'common_entity_attributes' in definitions:
        for attrib in definitions['common_entity_attributes']:
            common_entity_attributes.append(CommonAttribute(attrib['name'], attrib['type']))

    return namedtuple('general', ['common_entity_attributes', 'mandatory_components']) \
        (common_entity_attributes, mandatory_components)

def load_messages(definitions):
    if definitions is None:
        definitions = {}

    messages = OrderedDict()
    for (name, args) in definitions.items():
        if args == None:
            args = []
        messages[name] = Message(name, [(arg['name'], arg['type']) for arg in args])
    return messages

def load_components(definitions):
    if definitions is None:
        definitions = {}

    components = OrderedDict()
    for (name, kwargs) in definitions.items():
        if kwargs is None:
            kwargs = {}

        if not 'messages' in kwargs:
            kwargs['messages'] = []

        if not 'parameters' in kwargs:
            kwargs['parameters'] = OrderedDict()

        if not 'defaults' in kwargs:
            kwargs['defaults'] = OrderedDict()

        if not 'requires' in kwargs:
            kwargs['requires'] = OrderedDict()

        if not 'inherits' in kwargs:
            kwargs['inherits'] = OrderedDict()
        else:
            raise Exception("inherits not handled for now")

        convert_params(kwargs['defaults'])

        for component in kwargs['requires'].keys():
            if kwargs['requires'][component] is not None:
                convert_params(kwargs['requires'][component])

        components[name] = Component(name, **kwargs)
    return components

def load_entities(definitions):
    if definitions is None:
        definitions = {}

    entities = OrderedDict()
    for (name, kwargs) in definitions.items():
        if not 'components' in kwargs or kwargs['components'] == None:
            kwargs['components'] = OrderedDict()

        for component_name, component_params in kwargs['components'].items():
            if component_params != None:
                convert_params(component_params)
            else:
                kwargs['components'][component_name] = OrderedDict()

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
        component.gather_full_dependencies();

    for component in sorted_components:
        component.gather_messages(messages)

    for entity in entity_list:
        entity.gather_components(components, general.mandatory_components)

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


# A yaml loader that uses OrderedDict instead of dict internally.
class OrderedLoader(yaml.Loader):
    @staticmethod
    def OrderedMapping(loader, node):
        loader.flatten_mapping(node)
        return OrderedDict(loader.construct_pairs(node))

    def __init__(self, stream):
        yaml.Loader.__init__(self, stream)
        self.add_constructor(yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG, self.OrderedMapping)


if __name__ == '__main__':
    # Command line args
    parser = argparse.ArgumentParser(
        description="Outputs C++ plumbing code for the gamelogic given a component/entity definition file.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument('definitions', metavar='DEFINITION_FILE', type=open, help ="The definitions to use, - for stdin.")
    parser.add_argument('-t', '--template-dir', default="templates", type=str, help="Directory with template files.")
    parser.add_argument('-o', '--output-dir', default=None, type=str, help="Output directory for the generated source files.")
    parser.add_argument('-s', '--skeletons', action="store_true", help="Put latest skeleton files in a subfolder.")
    parser.add_argument('-p', '--header', default=None, type=argparse.FileType('r'), help="Prepend file contents as a header to all source files.")

    args = parser.parse_args()

    # Load everything from the definition file
    definitions = parse_definitions(yaml.load(args.definitions, OrderedLoader))
    args.definitions.close()

    # Manage output directories and files
    Output = namedtuple('Output', ['template', 'subdir', 'outname', 'overwrite'])

    subdir_names = {
        'backend':    'backend',
        'components': 'components',
        'skeletons':  'skeletons'
    }

    unique_files = {
        'backend':      Output('Backend.h',    subdir_names['backend'], 'CBSEBackend.h',    True),
        'backend_cpp':  Output('Backend.cpp',  subdir_names['backend'], 'CBSEBackend.cpp',  True),
        'components':   Output('Components.h', subdir_names['backend'], 'CBSEComponents.h', True),
        'entities':     Output('Entities.h',   subdir_names['backend'], 'CBSEEntities.h',   True),
        'helper':       Output('Helper.h',     '',                      'CBSE.h',           False)
    }

    template_params = [definitions, {
        'files': dict([(key, val.outname) for (key, val) in unique_files.items()]),
        'dirs': subdir_names
    }]

    if args.header != None:
        template_params += [{'header': args.header.read().strip().splitlines()}]

    if args.output_dir != None:
        base_dir       = args.output_dir + os.path.sep
        components_dir = base_dir + subdir_names['components'] + os.path.sep
        skeletons_dir  = components_dir + subdir_names['skeletons'] + os.path.sep

        # Generate a list of files to create, params_dicts will get squashed to create the template parameters
        FileToRender = namedtuple('FileToRender', ['template', 'output', 'params_dicts', 'overwrite'])
        to_render = []

        # Add unique files
        for (key, val) in unique_files.items():
            to_render.append(FileToRender(val.template, base_dir + val.subdir + os.path.sep + val.outname, \
                                          template_params, val.overwrite))

        # Add skeleton files
        for component in definitions['components']:
            params = template_params + [{'component': component}]

            if args.skeletons:
                to_render.append(FileToRender('Component.h', skeletons_dir + component.get_type_name() + '.h', params, True))
                to_render.append(FileToRender('Component.cpp', skeletons_dir + component.get_type_name() + '.cpp', params, True))

            to_render.append(FileToRender('Component.h', components_dir + component.get_type_name() + '.h', params, False))
            to_render.append(FileToRender('Component.cpp', components_dir + component.get_type_name() + '.cpp', params, False))

        # Now render the files
        env = jinja2.Environment(loader=PreprocessingLoader(args.template_dir), trim_blocks=True, lstrip_blocks=True)
        env.globals["enumerate"] = enumerate

        for render in to_render:
            if not render.overwrite and os.path.exists(render.output):
                continue

            params = OrderedDict()
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
