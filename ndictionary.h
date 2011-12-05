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

#ifndef _NPDICTIONARY_H_
#define _NPDICTIONARY_H_
 
#include "nsequence.h"
 
 typedef class NodeLocatorInfo            * NodeLocator;
 typedef class NodeDictionaryInfo         * NodeDictionary;
 typedef class KeyComparatorInfo          * KeyComparator;
 typedef class StrKeyComparatorInfo       * StrKeyComparator;
 typedef class IntKeyComparatorInfo       * IntKeyComparator;
  
  /********************************************************************
  * CLASS NAME: KeyComparator
  *  abstract class defining the interface for KeyComparators
  ********************************************************************/
 
#ifndef BOOL
#define BOOL int
#endif
 
class KeyComparatorInfo
{
  public:
  virtual BOOL equal( Object key1, Object key2 )=0;
   virtual char *printKey( Object key )=0;
};
 
/* integer comparator */
class IntKeyComparatorInfo: public KeyComparatorInfo
{
  public:
   virtual BOOL equal( Object key1, Object key2 ) { return ((int)key1 == (int)key2); }
   virtual char *printKey( Object key ) { return NULL; }
};
 
/* string comparator */
class StrKeyComparatorInfo: public KeyComparatorInfo
{
 public:
  virtual BOOL equal( Object key1, Object key2 );
  virtual char *printKey( Object key ) { return (char*)key; }
};
 
 
 /********************************************************************
  * CLASS NAME: NodeLocator
  *  extends NodePosition class to include a key for indexing
  ********************************************************************/
 
 class NodeLocatorInfo: public NodePositionInfo {
     private:
         Object m_key;
         
     public:
         NodeLocatorInfo( NodeSequence container, Object element, Object key ): 
            NodePositionInfo( container, element) 
         { m_key = key; };        

         void setKey( Object key ) { m_key = key; }
         Object key() { return m_key; }
 };
         
 /******************************************************************** 
  * CLASS NAME: NodeDictionary
  *  stores a series of locators that are acessed using their keys, 
  *  a key comparator class defines how keys are used
  ********************************************************************/
 
class NodeDictionaryInfo: public NodeSequenceInfo {
private:
    KeyComparator m_keyComp;
public:
    NodeDictionaryInfo( KeyComparator kC ): NodeSequenceInfo() { m_keyComp = kC; };
    
    /* appends the key, element pair to the end of the list */
    void        insert( Object element, Object key );
    /* if the key already exists, replaces the returned element, otherwise inserts new pair */
    Object      insertReplace( Object element, Object key );
    /* returns an object mapped to the key */
    Object      find( Object key );
    /* removes the key, element pair and returns the element */
    Object      remove( Object key );
    /* replaces the element mapped to the key */
    Object      replace( Object element, Object key );   
    /* debugging dump */
    void        dumpDictionary();
};
 
 
#endif
 
