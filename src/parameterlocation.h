#ifndef PARAMETERLOCATION_H
#define PARAMETERLOCATION_H

// Where an editable value lives, in terms that stay valid when the model
// rebuilds its internal structure (for example after a program is deleted
// and then restored by undo): which program, which group of controls, which
// control, which field. Undo commands store this instead of model pointers.
struct ParameterLocation
{
    enum class Group { Program, Pads, Knobs };

    int programId{-1};
    Group group{Group::Program};
    int controlId{0}; // pads and knobs only, 1-8
    int column{-1};
};

#endif // PARAMETERLOCATION_H
