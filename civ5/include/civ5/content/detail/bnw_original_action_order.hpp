#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>
namespace civ5::content::action_order_detail {
// Retail BNW DLL c5483b58, orderHotkeyInfo102E1350 -> sort102E0540.
// The public action permutation is part of the result. This repeats original
// pivot/equal-band/insertion/heap movement, not a host-STL stability choice.
template<class Item> class OriginalActionOrder final {
 static_assert(std::is_trivially_copyable_v<Item>,"Original action order requires copied scalar records");
 std::vector<Item>& a_;
 bool greater(std::size_t x,std::size_t y)const{return a_[x].priority>a_[y].priority;}
 void swap(std::size_t x,std::size_t y){if(x!=y)std::swap(a_[x],a_[y]);}
 void median3(std::size_t first,std::size_t middle,std::size_t last){
  //102D55E0: three actual comparisons, including the final check after swap.
  if(greater(middle,first))swap(first,middle);
  if(greater(last,middle))swap(middle,last);
  if(greater(middle,first))swap(first,middle);
 }
 void median(std::size_t first,std::size_t middle,std::size_t last){
  //102D6790 receives last-1. Its >40 distance is not total count>40.
  if(last-first>40){
   const auto step=(last-first+1)/8;
   median3(first,first+step,first+2*step);
   median3(middle-step,middle,middle+step);
   median3(last-2*step,last-step,last);
   median3(first+step,middle,last-step);
  }else median3(first,middle,last);
 }
 std::pair<std::size_t,std::size_t> partition(std::size_t first,std::size_t last){
  //102D68B0: retain and extend the contiguous pivot-equal band on both sides.
  const auto middle=first+(last-first)/2;median(first,middle,last-1);
  auto pfirst=middle,plast=pfirst+1;
  while(first<pfirst&&!greater(pfirst-1,pfirst)&&!greater(pfirst,pfirst-1))--pfirst;
  while(plast<last&&!greater(plast,pfirst)&&!greater(pfirst,plast))++plast;
  auto gfirst=plast,glast=pfirst;
  for(;;){
   while(gfirst<last){
    if(greater(pfirst,gfirst)){++gfirst;continue;}
    if(greater(gfirst,pfirst))break;
    swap(plast,gfirst);++plast;++gfirst;
   }
   while(first<glast){
    if(greater(glast-1,pfirst)){--glast;continue;}
    if(greater(pfirst,glast-1))break;
    --pfirst;swap(pfirst,glast-1);--glast;
   }
   if(glast==first&&gfirst==last)return {pfirst,plast};
   if(glast==first){
    if(plast!=gfirst)swap(pfirst,plast);
    ++plast;swap(pfirst,gfirst);++pfirst;++gfirst;
   }else if(gfirst==last){
    --glast;--pfirst;swap(glast,pfirst);--plast;swap(pfirst,plast);
   }else{--glast;swap(gfirst,glast);++gfirst;}
  }
 }
 void insertion(std::size_t first,std::size_t last){
  //102D6AE0 +10179040: equal priorities are not shifted in this leaf branch.
  if(first==last)return;
  for(auto pos=first+1;pos<last;++pos){
   const auto value=a_[pos];auto hole=pos;
   while(hole>first&&value.priority>a_[hole-1].priority){a_[hole]=a_[hole-1];--hole;}
   a_[hole]=value;
  }
 }
 void push_heap(std::size_t first,std::size_t hole,std::size_t top,Item value){
  //102D6830: parent comparison is strict; equal values stop bubbling.
  auto parent=hole?(hole-1)/2:0;
  while(hole>top&&a_[first+parent].priority>value.priority){
   a_[first+hole]=a_[first+parent];hole=parent;parent=hole?(hole-1)/2:0;
  }
  a_[first+hole]=value;
 }
 void adjust_heap(std::size_t first,std::size_t hole,std::size_t length,Item value){
  //102D6BC0: ties choose the right child (decrement only if right>left).
  const auto top=hole;auto child=2*hole+2;
  while(child<length){
   if(greater(first+child,first+child-1))--child;
   a_[first+hole]=a_[first+child];hole=child;child=2*child+2;
  }
  if(child==length){a_[first+hole]=a_[first+length-1];hole=length-1;}
  push_heap(first,hole,top,value);
 }
 void heap(std::size_t first,std::size_t last){
  //102D6C90 make_heap, then102D6F00 sort_heap. No generic STL heap is used.
  const auto length=last-first;
  for(auto parent=length/2;parent>0;){--parent;adjust_heap(first,parent,length,a_[first+parent]);}
  while(last-first>1){--last;const auto value=a_[last];a_[last]=a_[first];adjust_heap(first,0,last-first,value);}
 }
 void sort(std::size_t first,std::size_t last,std::size_t budget){
  //102E0540: recurse into smaller side, tail-process larger side.
  while(last-first>32&&budget>0){
   const auto band=partition(first,last);const auto half=budget/2;budget=half+half/2;
   if(band.first-first<last-band.second){sort(first,band.first,budget);first=band.second;}
   else{sort(band.second,last,budget);last=band.first;}
  }
  if(last-first<=32)insertion(first,last);else heap(first,last);
 }
 explicit OriginalActionOrder(std::vector<Item>& values):a_(values){}
 public:
 static void run(std::vector<Item>& values,std::size_t initial_budget){
  if(values.size()>static_cast<std::size_t>(INT32_MAX))throw std::runtime_error("Original action order source index overflow");
  OriginalActionOrder(values).sort(0,values.size(),initial_budget);
 }
 static void run(std::vector<Item>& values){run(values,values.size());}
};
}
