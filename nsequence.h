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

/*
 linked list implementation, by Matthew Baranowski
*/

#ifndef _NPSEQUENCE_H_
#define _NPSEQUENCE_H_

#define Object void *

#ifndef NULL
#define NULL 0
#endif

typedef class NodePositionInfo                  * NodePosition;
typedef class NodeSequenceInfo                  * NodeSequence;
                     
/********************************************************************
 * CLASS NAME: NodePosition
 * double linked nodeNodePosition implementation 
 ********************************************************************/


class NodePositionInfo {
    private:
        NodeSequence container_;
        Object    element_;
        NodePosition  nextNode_;
        NodePosition  prevNode_;
        
    public:
        NodePositionInfo( NodeSequence container, Object element )
                { container_ = container; element_ = element; };
        
        void setNextNode( NodePosition nextNode ) { nextNode_ = nextNode; };
        void setPrevNode( NodePosition prevNode ) { prevNode_ = prevNode; };
        NodePosition getNextNode() { return nextNode_; };
        NodePosition getPrevNode() { return prevNode_; };
        void setElement( Object element ) { element_ = element; };        
        NodeSequence container() { return container_; };
        Object    element() { return element_; };
};
        
/********************************************************************
 * 
 * CLASS NAME: NodeSequence
 *
 * double linked node sequence implementation
 * 
 ********************************************************************/


class NodeSequenceInfo {

    private:
        NodePosition first_;
        NodePosition last_;
        int size_;
        
    public:
        NodeSequenceInfo() { size_ = 0; first_ = last_ = NULL; };
		~NodeSequenceInfo();

		void clearSequence() { size_ = 0; first_ = last_ = NULL; };
        NodeSequence newContainer() { return new NodeSequenceInfo(); };
        int isEmpty() { if (size_ == 0) return 1; else return 0;};
        int size() { return size_; };
        Object replace(NodePosition, Object newElement );
        void  swap(NodePosition p1,NodePosition p2 );
        NodePosition first() { return first_; };
        NodePosition last() { return last_; };
        NodePosition before(NodePosition p );
        NodePosition after(NodePosition p );
        NodePosition insertFirst( Object element );
        NodePosition insertLast( Object element );
	NodePosition insertFirst( NodePosition nP );
	NodePosition insertLast( NodePosition nP );
        NodePosition insertBefore(NodePosition p, Object element );
        NodePosition insertAfter(NodePosition p, Object element );
        Object   remove(NodePosition p );
        Object   removeAfter(NodePosition p );
        Object   removeBefore(NodePosition p );
        Object   removeFirst();
        Object   removeLast();
	void dumpSequence();
};

#endif


    


