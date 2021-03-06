﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using MapWinGIS;
using MWLite.Core;
using MWLite.Core.Events;
using MWLite.Core.UI;

namespace MWLite.GUI.Forms
{
    public partial class GeoLocationForm : Form
    {
        public GeoLocationForm()
        {
            InitializeComponent();
            locationControl1.NewExtents += locationControl1_NewExtents;

            Shown += (s, e) => locationControl1.SetFocus();
        }

        void locationControl1_NewExtents(object sender, NewExtentsEventArgs e)
        {
            if (!e.Validate())
            {
                MessageHelper.Warn("Invalid extents.");
                return;
            }

            var map = App.Map;

            if (App.Map.GeoProjection.IsEmpty && App.Map.NumLayers == 0)
            {
                App.Map.Projection = tkMapProjection.PROJECTION_GOOGLE_MERCATOR;
            }

            switch (e.ExtentsType)
            {
                case ExtentType.Geogrpahic:
                    if (!map.SetGeographicExtents(e.GeographicExtents))
                    {
                        MessageHelper.Warn("Failed to set geographic extents: " + map.get_ErrorMsg(map.LastErrorCode));
                    }
                    break;
                case ExtentType.Projected:
                    map.Extents = e.ProjExtents;
                    break;
                case ExtentType.Known:
                    map.KnownExtents = e.KnownExtents;
                    break;
            }
        }
    }
}
