/*
 *	TVolumeUtils.java
 *
 *	This file is part of Tritonus: http://www.tritonus.org/
 */

/*
 *  Copyright (c) 1999 by Matthias Pfisterer
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as published
 *   by the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details.
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
|<---            this code is formatted to fit into 80 columns             --->|
*/

package org.tritonus.share.sampled;



public class TVolumeUtils
{
	private static final double		FACTOR1 = 20.0 / Math.log(10.0);
	private static final double		FACTOR2 = 1 / 20.0;



	public static double lin2log(double dLinear)
	{
		return FACTOR1 * Math.log(dLinear);
	}



	public static double log2lin(double dLogarithmic)
	{
		return Math.pow(10.0, dLogarithmic * FACTOR2);
	}
}



/*** TVolumeUtils.java ***/
