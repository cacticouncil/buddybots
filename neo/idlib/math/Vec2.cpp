/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"
#include "idlib/math/Angles.h"
#include "idlib/math/Matrix.h"
#include "idlib/Str.h"

#include "idlib/math/Vec2.h"

idVec2 vec2_origin( 0.0f, 0.0f );

namespace vec2
{
	PYBIND11_PLUGIN(idVec2) {
		py::module m("idVec2", "description");

		py::class_<idVec2>(m, "idVec2")
			.def(py::init<const float, const float>())
			.def_readwrite("x", &idVec2::x)
			.def_readwrite("y", &idVec2::y)
			.def("Set", &idVec2::Set)
			.def("Zero", &idVec2::Zero)
			.def("Length", &idVec2::Length)
			.def("LengthFast", &idVec2::LengthFast)
			.def("LengthSqr", &idVec2::LengthSqr)
			.def("Normalize", &idVec2::Normalize)
			.def("NormalizeFast", &idVec2::NormalizeFast)
			.def("Truncate", &idVec2::Truncate, py::return_value_policy::reference)
			.def("Clamp", &idVec2::Clamp)
			.def("Snap", &idVec2::Snap)
			.def("SnapInt", &idVec2::SnapInt)
			.def(-py::self)
			.def(py::self * py::self)
			.def(py::self * float())
			.def(py::self / float())
			.def(py::self + py::self)
			.def(py::self - py::self)
			.def(py::self += py::self)
			.def(py::self -= py::self)
			.def(py::self /= py::self)
			.def(py::self /= float())
			.def(py::self *= float())
			;

		return m.ptr();
	}
}

//===============================================================
//
//	idVec2
//
//===============================================================

/*
=============
idVec2::ToString
=============
*/
const char *idVec2::ToString( int precision ) const {
	return idStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}

/*
=============
Lerp

Linearly inperpolates one vector to another.
=============
*/
void idVec2::Lerp( const idVec2 &v1, const idVec2 &v2, const float l ) {
	if ( l <= 0.0f ) {
		(*this) = v1;
	} else if ( l >= 1.0f ) {
		(*this) = v2;
	} else {
		(*this) = v1 + l * ( v2 - v1 );
	}
}
