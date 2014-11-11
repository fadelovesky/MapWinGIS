﻿using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using AxMapWinGIS;
using MapWinGIS;

namespace MWLite.ShapeEditor
{
    static class MapEventHandlers
    {
        static AxMap _map = null;

        public static void Init(AxMap map)
        {
            _map = map;
            
            _map.AfterShapeEdit += _map_AfterShapeEdit;
            _map.BeforeDeleteShape += _map_BeforeDeleteShape;
            _map.BeforeShapeEdit += _map_BeforeShapeEdit;
            _map.MouseUpEvent += _map_MouseUpEvent;
            _map.ChooseLayer += _map_ChooseLayer;
            _map.ShapeValidationFailed += _map_ShapeValidationFailed;
            _map.UndoListChanged += _map_UndoListChanged;
            _map.ValidateShape += _map_ValidateShape;
            _map.PreviewKeyDown += _map_PreviewKeyDown;
        }

        static void _map_ChooseLayer(object sender, _DMapEvents_ChooseLayerEvent e)
        {
            e.layerHandle = App.Instance.Legend.SelectedLayer;
        }

        static void _map_PreviewKeyDown(object sender, PreviewKeyDownEventArgs e)
        {
            if (e.KeyCode == Keys.Escape)
            {
                if (!_map.ShapeEditor.IsEmpty)
                {
                    _map.ShapeEditor.Clear();
                }
            }
        }

        static void _map_BeforeDeleteShape(object sender, _DMapEvents_BeforeDeleteShapeEvent e)
        {
            string s = string.Empty;
            switch (e.target)
            {
                case tkDeleteTarget.dtShape: s = "shape";
                    break;
                case tkDeleteTarget.dtPart: s = "part";
                    break;
                case tkDeleteTarget.dtVertex: s = "vertex";
                    break;
            }

            s = string.Format("Do you want to delete {0}?", s);
            var result = MessageBox.Show(s, "Delete", MessageBoxButtons.YesNo, MessageBoxIcon.Question);
            e.cancel = result == DialogResult.No ? tkMwBoolean.blnTrue : tkMwBoolean.blnFalse;
        }

        static void _map_AfterShapeEdit(object sender, _DMapEvents_AfterShapeEditEvent e)
        {
            if (e.action == tkUndoOperation.uoRemoveShape)
            {
                MessageBox.Show("Shape was removed");
            }
        }

        static void _map_BeforeShapeEdit(object sender, _DMapEvents_BeforeShapeEditEvent e)
        {
            if (e.action == tkUndoOperation.uoRemoveShape)
            {

            }
        }

        static void _map_ValidateShape(object sender, _DMapEvents_ValidateShapeEvent e)
        {
            Debug.Print("Custom validation");
        }

        

        static void _map_ShapeValidationFailed(object sender, _DMapEvents_ShapeValidationFailedEvent e)
        {
            MessageBox.Show("Validation failed: " + e.errorMessage);
        }

        static void _map_MouseUpEvent(object sender, _DMapEvents_MouseUpEvent e)
        {
            if (e.button == 2)
            {
                if (_map.CursorMode == tkCursorMode.cmSelection)
                {
                    var menu = App.ContextMenu;
                    menu.Show(_map, e.x, e.y);
                }
            }
        }

        private static void _map_UndoListChanged(object sender, EventArgs e)
        {
            App.Instance.RefreshUI();
        }
    }
}
