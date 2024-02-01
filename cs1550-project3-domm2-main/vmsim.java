import java.util.*;
import java.io.*;
import java.util.Scanner;

public class vmsim{

    //32 or 64
    int bitmem; int pnum; int tratio; int ppts1; int ppts0;
    
    //print out vars + frames & page size 
    String alg; int pf; int dw; int ma;

    //from command args
    //vmsim -a <opt|lru> –n <numframes> -p <pagesize in KB> 
    //-s <memory split> <tracefile>
    String type; int frames; int pagesize; int split0; int split1; Scanner trace; String FileName;

    //from file
    String ls; int proc; long addr; int value; 

    //for write up 
    Scanner writeup;

    //one for each process
    LinkedList LRU1 = new LinkedList(); 
    LinkedList LRU0 = new LinkedList(); 
    LinkedList OPT1 = new LinkedList(); 
    LinkedList OPT0 = new LinkedList();

    Map<Long, LinkedList> OPTmap0 = new HashMap<Long, LinkedList>();
    Map<Long, LinkedList> OPTmap1 = new HashMap<Long, LinkedList>();

    public static void main(String[] args) throws IOException {
        // writeup = new Scanner(new FileInputStream(args[0]));
        // String[] fileArg = new String[5];
        // do{
        //     for(int i =0; i<fileArg.length; i++){
        //         String t = writeup.next();
        //         fileArg.add(t);
        //     }

        //     new vmsim(fileArg);
        //     writeup.nextLine();
        // }while(writeup.hasNextLine());
        
        new vmsim(args);
      }

    public vmsim(String[] args){
        //read in arg
        //takes account for the flags
        pf=0; dw=0; ma=0;
        type = args[1];
        if(type.equals("lru")) alg = "LRU";
        else if(type.equals("opt")) alg = "OPT";
        else { //System.out.println("invalid. enter lru or opt"); 
        return; }
        FileName = args[8];
        frames = Integer.parseInt(args[3]);
        pagesize = Integer.parseInt(args[5]);
        String temp = args[7];
        String[] temp2 = temp.split(":", 2); 
        split0 = Integer.parseInt(temp2[0]); 
        split1 = Integer.parseInt(temp2[1]);
        tratio = split0 + split1; 
        //add ratios together then divide ratio by that to get each process page table ratio
        //then multiple the num of frames by this to get the size of the page table (ppts) 
        //System.out.println("split0 "+split0+" ratio "+tratio);
        double dec1 = (double) split1 / tratio;  ppts1 = (int) (dec1 * frames); 
        double dec0 = (double) split0 / tratio;  ppts0 = (int) (dec0 * frames);
        try{
            trace = new Scanner(new FileInputStream(FileName));
            loadFile(type);
            //call Loadfile which will then call LRU or OPT    
        } catch(Exception e){
            print(); return;
        }
        
        //System.out.println(type + " hi" + frames + " " + pagesize + " " + split1 
                    //+ " " + split2);
        print();
    }

    public void print(){
        // Algorithm: LRU
        // Number of frames: 8
        // Page size: 8 KB
        // Total memory accesses: %d
        // Total page faults: %d
        // Total writes to disk: %d
        System.out.println("Algorithm: " + alg);
        System.out.println("Number of frames: " + frames);
        System.out.println("Page size: " + pagesize + " KB");
        System.out.println("Total memory accesses: " + ma);
        System.out.println("Total page faults: " + pf);
        System.out.println("Total writes to disk: " + dw);
    }

    public void LRU(String ls, long addr, int proc){

        
        // if(LRU0.head == null) System.out.println("NO LINKED LIST");
        // else{
        //     Node B = LRU0.head;
        //     boolean cond = true;
        //     do{
        //         System.out.println("b4 add -0- LINKED LIST NODE ADDRESS " + B.addr);

        //         if(B.next != null){ B = B.next; }
        //         else cond = false;
        //     }while(cond);
        // }
        // if(LRU1.head == null) System.out.println("NO LINKED LIST");
        // else{
        //     Node t = LRU1.head;
        //     boolean cond = true;
        //     do{
        //        System.out.println("b4 add -1- LINKED LIST NODE ADDRESS " + t.addr);

        //         if(t.next != null){ t = t.next; }
        //         else cond = false;
        //     }while(cond);
        // }

        int dirty = 0; 
        //set dirty bit 
        if(ls.equals("s")) dirty = 1;

        //run through 
        if(proc == 0) {
            Node n = findMatch(LRU0, addr);
            if(n != null){
                if(n.dirty==1) dirty=1;
                LRU0.add(addr, dirty, ppts0, n);
                ma+=1;
            }
            else { 
                if((LRU0.size >= ppts0) && LRU0.head.dirty ==1) dw +=1;
                LRU0.add(addr, dirty, ppts0);
                ma+=1;
                pf+=1; 
                ////System.out.println("---- incrementing page fault IT JUS HAPPENED----" + pf);
            }
        }
        else if(proc == 1) {
            Node n = findMatch(LRU1, addr);
            if(n != null){
                if(n.dirty==1) dirty=1;
                LRU1.add(addr, dirty, ppts1, n);
                ma+=1;
            }
            else {
                if((LRU1.size >= ppts1) && LRU1.head.dirty ==1) dw +=1;
                LRU1.add(addr, dirty, ppts1); 
                ma+=1;
                pf+=1; 
                ////System.out.println("---- incrementing page fault IT JUS HAPPENED----" + pf);
            }
        }
        
        //System.out.println("mem " + res[0] + " fault " + res[1]+" disk "+res[2]);
        return;
    }

    public void OPT(){
        //OPT remove the farthest away in future node 
        //traverse through the file and for each process add to the hash table  
            //addr as the key
            //values are each line that accessed it

        //now traverse through again and for each process 
        //cross off its access value in the hash table 
        //if linkedlist is empty: add to the linked list 
        //if linkedlist is full: run opt 
            //go thorugh the linkedlist and each node check 
            //the hashtable for the biggest num and store the node
            //if there is no number left it not accessed and is the biggest num
            //this is the node you remove and then add the new to tail
        //if there is a tie you remove by LRU 
        
        try{
            trace = new Scanner(new FileInputStream(FileName));            
        } catch(Exception e){
            System.out.println("File not found.");
        }
        int dirty1;
        do{
            ls = trace.next(); 
            String addtemp1 = trace.next(); 
            addtemp1 = addtemp1.substring(2); 
            if(addtemp1.length()>8) bitmem = 64; 
            else bitmem = 32; 
            addr = Long.parseUnsignedLong(addtemp1, 16); 
            int psbyt1 = pagesize * 1024; 
            int offset1 = (int)(Math.log(psbyt1) / Math.log(2)); 
            addr = addr >>> offset1; 
            proc = trace.nextInt(); 
            if(ls.equals("s")) dirty1 = 1; 
            else dirty1 = 0;
            //---------------------------------------------------------------------
            //System.out.println(ls + " " + addr + " " + proc); 
            // if(OPT0.head == null) System.out.println("NO LINKED LIST");
            // else{
            //     Node B = OPT0.head;
            //     boolean cond = true;
            //     do{
            //         System.out.println("b4 add -0- LINKED LIST NODE ADDRESS " + B.addr + " dirty bit: " + B.dirty);

            //         if(B.next != null){ B = B.next; }
            //         else cond = false;
            //     }while(cond);
            // } 
            // if(OPT1.head == null) System.out.println("NO LINKED LIST");
            // else{
            //     Node B = OPT1.head;
            //     boolean cond = true;
            //     do{
            //         System.out.println("b4 add -0- LINKED LIST NODE ADDRESS " + B.addr  + " dirty bit: " + B.dirty);

            //         if(B.next != null){ B = B.next; }
            //         else cond = false;
            //     }while(cond);
            // } 
            
            if(proc == 0) {
                //OPTmap0.get(addr).remove();
                //if match jus update the dirty bit 
                Node OPTmatch = findMatch(OPT0, addr);
                //System.out.println(OPTmatch + " match ");
                if(OPTmatch != null){
                    if(OPTmatch.dirty==1) dirty1=1;
                    OPT0.add(addr, dirty1, ppts0, OPTmatch);
                    ma+=1;
                   // System.out.println( " dirty or not ");
                }

                else if(OPT0.size<ppts0){
                    OPT0.add(addr, dirty1, ppts0);
                    ma+=1;
                    pf+=1;
                    //System.out.println(pf + " pf ");
                }
                else{
                    //find farthest in helper returns the node within linked list 
                    Node m = OPThelper(OPT0, OPTmap0);
                    //System.out.println(m.addr + " addr being deleted by opt alg. dirty? " + m.dirty);
                    if(m.dirty == 1) dw +=1;
                    //System.out.println(dw + " dw ");
                    pf+=1;
                    //System.out.println(pf + " pf ");
                    //delete and add from linkedlist OPT0
                    OPT0.add(addr, dirty1, ppts0, m);
                    ma+=1;
                }
                //System.out.println("key: " + OPTmap0.get(addr) + " addr: " + addr);
                // if(OPTmap0.get(addr).head != null){ System.out.println("b4 remove hashmap0----- "+ OPTmap0.get(addr).head.addr ); }
                // else {System.out.println("b4 remove hashmap0----- head is null");}
                OPTmap0.get(addr).remove();
                // if(OPTmap0.get(addr).head != null){ System.out.println("after remove hashmap0----- "+ OPTmap0.get(addr).head.addr ); }
                // else {System.out.println("after remove hashmap0----- head is null");}
            }
            else if(proc == 1) {
                //OPTmap1.get(addr).remove();
                Node OPTmatch = findMatch(OPT1, addr);
                if(OPTmatch != null){
                    if(OPTmatch.dirty==1) dirty1=1;
                    OPT1.add(addr, dirty1, ppts1, OPTmatch);
                    ma+=1;
                }

                else if(OPT1.size<ppts1){
                    OPT1.add(addr, dirty1, ppts1);
                    ma+=1;
                    pf+=1;
                   // System.out.println(pf + " pf ");
                }
                else{
                    //find farthest in helper returns the node within linked list 
                    Node m = OPThelper(OPT1, OPTmap1);
                    if(m.dirty == 1) dw +=1;
                    pf+=1;
                    //delete and add from linkedlist OPT0
                    OPT1.add(addr, dirty1, ppts1, m);
                    ma+=1;
                }
                //System.out.println("key: " + OPTmap1.get(addr) + " addr: " + addr );
                // if(OPTmap1.get(addr).head != null){ System.out.println("b4 remove hashmap1----- "+ OPTmap1.get(addr).head.addr ); }
                // else {System.out.println("b4 remove hashmap1----- head is null");}
                OPTmap1.get(addr).remove();
                // if(OPTmap1.get(addr).head != null){ System.out.println("after remove hashmap0----- "+ OPTmap1.get(addr).head.addr ); } //+ " next "+ OPTmap1.get(addr).head.next.addr
                // else {System.out.println("after remove hashmap0----- head is null");}
            }
            trace.nextLine();
        }while(trace.hasNextLine());
        //System.out.println("mem " + ma + " fault " + pf + " disk " + dw);
        
        trace.close();
        return;
    }
    //return node to delete 
    public Node OPThelper(LinkedList L, Map<Long, LinkedList> OPTmap){
        if(L.head == null) return null;
        Node n = L.head;
        long far = 0;
        Node remov = null;
        boolean cond = true;

        do{
            // if(OPTmap.get(n.addr) == null){
            //     return n; 
            // }
            if(OPTmap.get(n.addr).head == null){
                return n; 
            }
            long now = OPTmap.get(n.addr).head.addr;
            if(now > far) {
                far = now; remov = n; 
            }
            if(n.next != null){ n = n.next; }
            else cond = false;
        }while(cond);
        return remov;
        //tail to head 
         // while(n != null){
        //     if(OPTmap.get(n.addr).size() <= 0 || OPTmap.get(n.addr).get(0) > far){
        //         if(OPTmap.get(n.addr).size()>0)
        //             far = OPTmap.get(n.addr).get(0);
        //         remov = n; 
        //     }
        //      n = n.prev; 
        //  }
    }

    public Node findMatch(LinkedList L, long addr){
        if(L.head == null) return null;
        Node n = L.head;
        boolean cond = true;
        do{
            if(n.addr == addr) return n;

            if(n.next != null){ n = n.next; }
            else cond = false;
        }while(cond);

        return null;
    }

    public void loadFile(String type){
        //read in file by line
        //System.out.println("here");
        //Scanner fileScan = new Scanner(new FileInputStream(trace)); 
        //boolean cond = true;
        value =0;
        do{
            //load or store
            ls = trace.next(); 
            String addtemp = trace.next(); 
            addtemp = addtemp.substring(2); 
            //System.out.println("addtemp hex "+ addtemp);

            //find size of addr to set the bitmem size 
            if(addtemp.length()>8) bitmem = 64; 
            else bitmem = 32; 
            addr = Long.parseUnsignedLong(addtemp, 16); 

            //turn page size into bytes (4kb = 4000 bytes)
            int psbyt = pagesize * 1024; 
            //find page offset from page size exponent
            int offset = (int)(Math.log(psbyt) / Math.log(2)); 
            //set page number 
            pnum = bitmem - offset; 
           // offset = offset / 4; 

            //remove offset so now it is just page num 
            //this gets stored in linkedlist as physical mem 
            addr = addr >>> offset; 
            proc = trace.nextInt(); 
            //System.out.println(ls + " " + addr + " " + proc); 
            
            //calls one of the paging algorithms and feeds the page access vars
            if(alg.equals("LRU")) LRU(ls, addr, proc); 
            else if(alg.equals("OPT")) {
                if(proc == 0){
                    if(OPTmap0.get(addr) == null) {
                        OPTmap0.put(addr, new LinkedList());
                    }
                    OPTmap0.get(addr).add(value, 0, Integer.MAX_VALUE);
                    // int re = 0; 
                    // if(OPTmap0.containsKey(addr))  re  = 1; 
                    //System.out.println( re + " in " + addr + " " + OPTmap0.get(addr).indexOf(Integer.valueOf(value)) + " inserted in HASHMAP ------");

                }
                else if(proc == 1){
                    if(OPTmap1.get(addr) == null) {
                        OPTmap1.put(addr, new LinkedList());
                    }
                    OPTmap1.get(addr).add(value, 0, Integer.MAX_VALUE);
                    // int re = 0; 
                    // if(OPTmap1.containsKey(addr))  re = 1; 
                    //System.out.println( re + " in " + addr + " " + OPTmap1.get(addr).indexOf(Integer.valueOf(value)) + " inserted in HASHMAP ------");
                }
                value += 1;
            } 
            else { 
                //System.out.println("invalid. enter lru or opt"); 
                return; }
            trace.nextLine();
            
        }while(trace.hasNextLine());
        value = 0;
        trace.close();
        if(alg.equals("OPT")) OPT();  
        //System.out.println("made it out finally ");
       
        return; 
    }
    //./vmsim -a <opt|lru> –n <numframes> -p <pagesize in KB> -s <memory split> <tracefile>

    //each line is a page access  
    // - first letter is l (load) or s (store) store is a modification and makes the bit dirty 
    // - virtual address in 32 bits 
    // - last number determines which process is making the access 

    //# page faults
    //# tht memory has been accessed 

    //numframes = linked list length 
    //memory split : is how the frames get split up between the processes 

    //turn page size to bytes (4kb = 2^12 bytes)
    //subtract exponent from address space (32 bits - 12 ) = page number = 20 
    //first 20 bits are page number and need stored in the linked list as physical mem
    //12 = page offset = last, far right numbers of the virtual address, dont need these nums 
    //logical shift  >>>

    //LRU remove the head and add a tail 
        // - if the element is in the list but needs to move up dont count as a page fault 
    //OPT remove the farthest away in future node 
        //traverse through the file and for each process add to the hash table  
            //addr as the key
            //values are each line that accessed it
        //now traverse through again and for each process 
            //cross off its access value in the hash table 
            //if linkedlist is empty: add to the linked list 
            //if linkedlist is full: run opt 
                //go thorugh the linkedlist and each node check 
                //the hashtable for the biggest num and store the node
                //if there is no number left it not accessed and is the biggest num
                //this is the node you remove and then add the new to tail
        //if there is a tie you remove by LRU 


    //Disk write is when you replace a dirty bit 
    //page fault is when it was not already in memory 
    //becomes dirty when a store occurs

    /*print 
        Algorithm: LRU
        Number of frames: 8
        Page size: 8 KB
        Total memory accesses: %d
        Total page faults: %d
        Total writes to disk: %d
    */
}