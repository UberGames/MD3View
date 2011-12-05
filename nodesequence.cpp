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
#include <string.h>
#include <iostream>

using namespace std; 

//void DebugToFile (char *msg, ...);

BOOL StrKeyComparatorInfo::equal( Object key1, Object key2 )
{
 return (!strcmp( (char*)key1, (char*)key2 )); 
}


/*
=======
destructor removes sequence positions, however does nothing for the object pointers
use this if _only_ if don't need to delete position elements
=======
*/
NodeSequenceInfo::~NodeSequenceInfo()
{
	NodePosition pos, next;
	if (!isEmpty()) {
		for(pos=first(); pos!=NULL ; pos=next) {
			next = after(pos);
			remove(pos);
		}
	}
}


Object NodeSequenceInfo::replace(NodePosition position, Object newElement )
{
    NodePosition nodePosition = (NodePosition)position;
    Object toReturn = nodePosition->element();
    nodePosition->setElement( newElement );
    return toReturn;
};

void NodeSequenceInfo::dumpSequence()
{
        
  cout << "Sequence Dump, size " <<  size() << endl;  
  for (NodePosition pos = first(); pos != NULL; pos = after(pos) ) {
    cout << "   Position " << pos;
    cout << " next " << pos->getNextNode();
    cout << " prev " << pos->getPrevNode();
    cout << " element " << pos->element() << endl;
  }
                      
}

void NodeSequenceInfo::swap(NodePosition p1,NodePosition p2 )
{
    NodePosition nP1 = (NodePosition)p1;
    NodePosition nP2 = (NodePosition)p2;
    Object element = nP1->element();
    nP1->setElement( nP2->element() );
    nP2->setElement( element );
};

NodePosition NodeSequenceInfo::before( NodePosition p )
{
    NodePosition nodePosition = (NodePosition)p;
    return nodePosition->getPrevNode();
};

NodePosition NodeSequenceInfo::after( NodePosition p )
{
    NodePosition nodePosition = (NodePosition)p;
    return nodePosition->getNextNode();
};

NodePosition NodeSequenceInfo::insertFirst( Object element )
{
    NodePosition nP = new NodePositionInfo( this, element );
    return insertFirst( nP );
}

NodePosition NodeSequenceInfo::insertFirst( NodePosition nP )
{
    nP->setPrevNode( NULL );
    nP->setNextNode( first_ );    
    if (first_) first_->setPrevNode( nP );
    first_ = nP;
    if (nP->getNextNode() == NULL) {
        last_ = nP;
    }
    
    size_++;
    return first_;
}

NodePosition NodeSequenceInfo::insertLast( Object element )
{
    NodePosition nP = new NodePositionInfo( this, element );
    return insertLast( nP );
};


NodePosition NodeSequenceInfo::insertLast( NodePosition nP )
{
    nP->setPrevNode( last_ );
    nP->setNextNode( NULL );
    if (last_) last_->setNextNode( nP );
    last_ = nP;
    if (nP->getPrevNode() == NULL) {
        first_ = nP;
    }
    size_++;
    return last_;
};


NodePosition NodeSequenceInfo::insertAfter(NodePosition p, Object element )
{       
    NodePosition node = (NodePosition)p;
    if (node->getNextNode() == NULL) {
                NodePosition result = insertLast(element);                              
        return result;
    }

    NodePosition nP = new NodePositionInfo( this, element );
    nP->setPrevNode( node );
    nP->setNextNode( node->getNextNode() );
    node->getNextNode()->setPrevNode( nP );
    node->setNextNode( nP );
    size_++;    
    return nP;
        
};

NodePosition NodeSequenceInfo::insertBefore(NodePosition p, Object element )
{
    NodePosition node = (NodePosition)p;
    if (node->getPrevNode() == NULL) {
        return insertFirst( element );
    }

    NodePosition nP = new NodePositionInfo( this, element );
    nP->setNextNode( node );
    nP->setPrevNode( node->getPrevNode() );
    NodePosition prevNode = node->getPrevNode();
    prevNode->setNextNode( nP );
    node->setPrevNode( nP );
    size_++;
    return nP;
};

Object NodeSequenceInfo::remove(NodePosition p )
{
    NodePosition nP = (NodePosition)p;
    Object toReturn = nP->element();

    NodePosition prevNode = nP->getPrevNode();

    if (prevNode == NULL) {
        Object result = removeFirst();                
        return result;
    }
    
    NodePosition nextNode = nP->getNextNode();

    if (nextNode == NULL) { 
            Object result = removeLast();
                return result;

    }
  
    prevNode->setNextNode( nextNode );
    nextNode->setPrevNode( prevNode );
    delete nP;
    size_--;
    return toReturn;
};

Object NodeSequenceInfo::removeAfter(NodePosition p )
{

    NodePosition nP = (NodePosition)p;

    if (nP->getNextNode() == NULL) {
        return NULL;
    }
    nP = nP->getNextNode();
    
    Object toReturn = nP->element();

    NodePosition prevNode = nP->getPrevNode();    
    NodePosition nextNode = nP->getNextNode();

    if (nextNode == NULL) {
        return removeLast();
    }
    
    prevNode->setNextNode( nextNode );
    nextNode->setPrevNode( prevNode );
    delete nP;
    size_--;
    return toReturn;
};

Object NodeSequenceInfo::removeBefore(NodePosition p )
{
    NodePosition nP = (NodePosition)p;

    if (nP->getPrevNode() == NULL) {
        return NULL;
    }
    nP = nP->getPrevNode();
    
    Object toReturn = nP->element();

    NodePosition prevNode = nP->getPrevNode();

    if (prevNode == NULL) {
        return removeFirst();
    }
    
    NodePosition nextNode = nP->getNextNode();
    
    prevNode->setNextNode( nextNode );
    nextNode->setPrevNode( prevNode );
    delete nP;
    size_--;
    return toReturn;
};

Object NodeSequenceInfo::removeFirst()
{
    NodePosition nP = (NodePosition)first_;
    Object toReturn = nP->element();
    
    NodePosition nextNode = nP->getNextNode();
    if (nextNode) nextNode->setPrevNode( NULL );
    first_ = nextNode;
    if (!first_) last_ = NULL;
    delete nP;
    size_--;
    return toReturn;
};

Object NodeSequenceInfo::removeLast()
{
    NodePosition nP = (NodePosition)last_;
    Object toReturn = nP->element();
    
    NodePosition prevNode = nP->getPrevNode();
    if (prevNode) prevNode->setNextNode( NULL );
    last_ = prevNode;
    if (!last_) first_ = NULL;
    delete nP;
    size_--;
    return toReturn;
};
