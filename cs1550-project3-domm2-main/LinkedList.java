public class LinkedList{
    Node head;
    Node tail;
    int size;
    int m;
    private int pts;
    private Node match; 

    public LinkedList(){
        size = 0; 
        head = null;
        tail = null;
        pts = 0;
        match = new Node();
        m = 0;
    }
    //add when match
    public void add(long addr, int dirty, int ppts, Node match){
        pts = ppts; 

        Node node = new Node();
        node.addr = addr; 
        node.dirty = dirty;
        this.match = match;
        boolean matched = true;

       //-System.out.println("a match and size "+size+" pts "+pts);
       //if(node.dirty == 1) d += 1; //disk write
       // item exists so we call replace 
        replaceHelper(matched, node);  
    
        if(head == null){} ////System.out.println("NO LINKED LIST+++++");
        else{
            Node c = head;
            boolean cond = true;
            do{
                //-System.out.println("++++LINKED LIST NODE ADDRESS " + c.addr);

                if(c.next != null){ c = c.next; }
                else cond = false;
            }while(cond);
        }
        return; 
    }
    //add when not a match 
    public void add(long addr, int dirty, int ppts){
        pts = ppts; 
        m=0;
        Node node = new Node();
        node.addr = addr; 
        node.dirty = dirty;
        boolean matched = false;
    
        //if table is full call replace 
        if(size >= pts){
            //-System.out.println("fullll size "+size+" pts "+pts);
            // if(node.dirty == 1) d += 1; //disk write
            replaceHelper(matched, node);
        }
        //else call add 
        else{
           //- System.out.println("spaceeee size "+size+" pts "+pts);
            addHelper(node);
        }
        // if(head == null){} //System.out.println("NO LINKED LIST-----");
        // else{
        //     Node y = head;
        //     boolean cond = true;
        //     do{
        //        //- System.out.println("------LINKED LIST NODE ADDRESS " + y.addr);

        //         if(y.next != null){ y = y.next; }
        //         else cond = false;
        //     }while(cond);
        // }
        return; 
    }

    //add helper. table not full and node not there. doing page fault
    private void addHelper(Node node){

        if(head == null){
            head = node; 
            tail = node; //added for opt
            size += 1; m += 1; 
            return;
        }
        else if(head.next == null){
            tail = node;
            tail.prev = head;

            head.next = tail; 

            //node.prev = head; //dont need
            //tail.next = null;
            size += 1; m += 1; 
            return;
        }
        //at least two nodes && size < pts 
       //- System.out.println("- - - - - - - - SIZE " +size+" pts "+pts);
        if(size < pts){
            
            Node temp = new Node();
            temp = tail;
            
            tail = node;
            tail.prev = temp;
            tail.prev.next = tail; //or temp.next = node
            size += 1;
            m += 1; 
            return;
        }
        //System.out.println("- - - - - - - -NONE");
        return;
    }//end add()

    //when full or theres a match we delete then add 
    private void replaceHelper(boolean matched, Node node){
        //if no match but table full or match is the head 
        //delete head and add tail
        if(!matched || match == head){
            del(); 
            addHelper(node);
            return;
        }
        //if match, delete match and add
        else{
            //do u have a diskwrite when moving a dirty node to front?  
            //if(match.dirty == 1) d += 1; //disk write
            delmatch(); 
            addHelper(node);
            return;
        }
    }

    public void delmatch(){
        if(head==null||match==null) return;
        if(match.next == null){
            tail = match.prev;
            tail.next = null; 
            size -= 1;
            return;
        }
        match.prev.next = match.next;
        match.next.prev = match.prev;
        size -= 1;
        return;
    }
    //del head / push head off
    private void del(){
        if(head == null) return;
        if(head.next == null){
            head = null; tail = null; size -= 1; return;
        }
        head = head.next;
        head.prev = null;
        size -= 1;
        return;
    }//end del()
    public void remove(){
        if(head == null) return;
        if(head.next == null){
            head = null; tail = null; size -= 1; return;
        }
        head = head.next;
        head.prev = null;
        size -= 1;
        return;
    }//end del()

}
