/*
 // Copyright (c) 2023 Alexander Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "../PluginEditor.h"
#include "Buttons.h"

String MainToolbarButton::getTooltip()
{
    auto setTooltip = TextButton::getTooltip();
    if (auto editor = dynamic_cast<PluginEditor*>(getParentComponent())) {
        auto cnv = editor->getCurrentCanvas();
        if (isUndo) {
            setTooltip = "Undo";
            if (getValue<bool>(editor->canUndo) && cnv->patch.lastUndoSequence != "")
                setTooltip += ": " /* + cnv->patch.getTitle() + ": " */ + cnv->patch.lastUndoSequence;
        } else if (isRedo) {
            setTooltip = "Redo";
            if (getValue<bool>(editor->canRedo) && cnv->patch.lastRedoSequence != "")
                setTooltip += ": "  /* + cnv->patch.getTitle() + ": " */ + cnv->patch.lastRedoSequence;
        }
    }
    return setTooltip;
}