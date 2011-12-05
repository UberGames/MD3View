/*
Copyright (C) 2010 Matthew Baranowski, Sander van Rossen & Raven software.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "ndictionary.h"
#include <iostream>

using namespace std; 

void NodeDictionaryInfo::insert( Object element, Object key )
{
        NodeLocator nL = new NodeLocatorInfo( this, element, key );
        insertLast( nL );
};


Object NodeDictionaryInfo::insertReplace( Object element, Object key )
{
	Object toReturn;
		NodeLocator nL;
        for (nL = (NodeLocator)first(); nL != NULL; nL = (NodeLocator)after(nL))
        {
                if (m_keyComp->equal( nL->key(), key )) {
		  toReturn = nL->element();
		  nL->setElement( element );
                  return toReturn;
                }
        }

	// found nothing so append
	nL = new NodeLocatorInfo( this, element, key );
        insertLast( nL );
        return NULL;
};


Object NodeDictionaryInfo::find(Object key)
{
        for (NodeLocator nL = (NodeLocator)first(); nL != NULL; nL = (NodeLocator)after(nL))
        {       
                if (m_keyComp->equal( nL->key(), key )) {
                        return nL->element();
                }
        }       
        return NULL; // found nothing   
}

Object NodeDictionaryInfo::remove( Object key )
{
        for (NodeLocator nL = (NodeLocator)first(); nL != NULL; nL = (NodeLocator)after(nL))
        {
                if (m_keyComp->equal( nL->key(), key )) {
                        return remove( nL );                                            
                }
        }       
        return NULL; // found nothing   
}


Object NodeDictionaryInfo::replace( Object element, Object key )
{
  Object toReturn;
        for (NodeLocator nL = (NodeLocator)first(); nL != NULL; nL = (NodeLocator)after(nL))
        {
                if (m_keyComp->equal( nL->key(), key )) {
		  toReturn = nL->element();
		  nL->setElement( element );
                  return toReturn;                                         
                }
        }       
        return NULL; // found nothing   
}


void NodeDictionaryInfo::dumpDictionary()
{      
  cout << "Dictionary Dump, size " <<  size() << endl; 
  for (NodeLocator nL = (NodeLocator)first(); nL != NULL; nL = (NodeLocator)after(nL))
  {
    cout << "           Position " << nL; 
    cout << " next " << nL->getNextNode();
    cout << " prev " << nL->getPrevNode();
    cout << " element " << nL->element();
    cout << " key " << (char *)nL->key() << endl;
  }   
}
