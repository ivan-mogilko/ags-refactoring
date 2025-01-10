using System;
using System.Collections.Generic;
using System.Text;
using System.Drawing;

namespace AGS.Types
{
    public struct AGSColor
    {
        /// <summary>
        /// Default 32-bit transparent color used in AGS.
        /// </summary>
        public static int ColorNumberTransparent { get { return 0; } }
        /// <summary>
        /// Default palette index used for transparent color.
        /// </summary>
        public static int PaletteIndexTransparent { get { return 0; } }

        public static IColorMapper ColorMapper { get; set; }

        private int _agsColorNumber;

        public AGSColor(int agsColorNumber)
        {
            _agsColorNumber = agsColorNumber;
        }

        public AGSColor(Color rgbColor)
        {
            _agsColorNumber = ColorMapper.MapRgbColorToAgsColourNumber(rgbColor);
        }

        public int ColorNumber { get { return _agsColorNumber; } }

        public Color ToRgb()
        {
            return ColorMapper.MapAgsColourNumberToRgbColor(_agsColorNumber);
        }

    }
}
