/*********************************************************************************/
/* This file has been written by Sergio Perez Alcañiz <serpeal@disca.upv.es>     */
/*            Departamento de Informática de Sistemas y Computadores             */
/*            Universidad Politécnica de Valencia                                */
/*            Valencia (Spain)                                                   */
/*                                                                               */
/* The RTL-lwIP project has been supported by the Spanish Government Research    */
/* Office (CICYT) under grant TIC2002-04123-C03-03                               */
/*                                                                               */
/* Copyright (c) March, 2003 SISTEMAS DE TIEMPO REAL EMPOTRADOS, FIABLES Y       */
/* DISTRIBUIDOS BASADOS EN COMPONENTES                                           */
/*                                                                               */
/*  This program is free software; you can redistribute it and/or modify         */
/*  it under the terms of the GNU General Public License as published by         */
/*  the Free Software Foundation; either version 2 of the License, or            */
/*  (at your option) any later version.                                          */
/*                                                                               */
/*  This program is distributed in the hope that it will be useful,              */
/*  but WITHOUT ANY WARRANTY; without even the implied warrabnty of              */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                */
/*  GNU General Public License for more details.                                 */
/*                                                                               */
/*  You should have received a copy of the GNU General Public License            */
/*  along with this program; if not, write to the Free Software                  */
/*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA    */
/*                                                                               */
/*  Linking RTL-lwIP statically or dynamically with other modules is making a    */
/*  combined work based on RTL-lwIP.  Thus, the terms and conditions of the GNU  */
/*  General Public License cover the whole combination.                          */
/*                                                                               */
/*  As a special exception, the copyright holders of RTL-lwIP give you           */
/*  permission to link RTL-lwIP with independent modules that communicate with   */
/*  RTL-lwIP solely through the interfaces, regardless of the license terms of   */
/*  these independent modules, and to copy and distribute the resulting combined */
/*  work under terms of your choice, provided that every copy of the combined    */
/*  work is accompanied by a complete copy of the source code of RTL-lwIP (the   */
/*  version of RTL-lwIP used to produce the combined work), being distributed    */
/*  under the terms of the GNU General Public License plus this exception.  An   */
/*  independent module is a module which is not derived from or based on         */
/*  RTL-lwIP.                                                                    */
/*                                                                               */
/*  Note that people who make modified versions of RTL-lwIP are not obligated to */
/*  grant this special exception for their modified versions; it is their choice */
/*  whether to do so.  The GNU General Public License gives permission to        */
/*  release a modified version without this exception; this exception also makes */
/*  it possible to release a modified version which carries forward this         */
/*  exception.                                                                   */
/*********************************************************************************/

#include "netif/ethernetif.h"

#define COM3_905C_NAME "eth"

struct netif;

err_t rt_3c905cif_init(struct netif *netif);
void rt_3c905cif_close(void);

