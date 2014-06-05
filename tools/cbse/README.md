Unvanquished CBSE
=================

Testing auto generation of plumbing code for a component-based gamelogic for the game Unvanquished.

Please, please keep in mind that this is *very* WIP.

Terminology
-----------

An *entity* is an object of the game that contains a number of behaviors called *components* that interact using entity-wide broadcast *messages* and shared *attributes* that are read-only variable with an added broadcast "set" message.
Because using only messages and attributes is a bit limited, we support component dependencies and component inheritance.
When a component depends on another, it can call methods on the other directly.
In addition inheritance allows to surcharge some message handling as well as methods directly with a dependency.

How the generation works
------------------------

A python tool reads a YAML definition of the components/entities... does processing and calls Jinja2 templates to "render" C++ files.
As part of the processing, the correctness of the definition should be checked (for example the dependency-inheritance graph must be acyclic) and each component will gather its "own" attributes/messages... for rendering.

How the generated code works
----------------------------

Each entity contains a pointer to each component and has a virtual function that dispatches the messages to the right components (known statically since the code is auto-generated).
It also contains the shared attributes.
Each component contains a pointer to the entity, pointers to component dependencies and shared attributes.

Each component is implemented as a "stub" class that only contains helper functions, the customer code will be able to us that class as such
```c++
class MyComponent: protected BaseMyComponent {
    // the constructor will probably be forced :/
    // but otherwise you can use helper functions here
}
```
Inheritance is handled as such, if AliceComponent is the parent of BobComponent then the tool will generate a stub class for Alice and a stub class for Bob that doesn't contain the properties of Alice, then the customer code is like this:

```c++
class AliceComponent: protected BaseAliceComponent {
    //Stuff
}

class BobComponent: protected AliceComponent, BaseBobComponent {
    //Moar stuff
}
```
