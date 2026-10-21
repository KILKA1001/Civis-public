 // Repeated genuine5F7B80 GetPreGame factory in the retained singleton heap.
 // Each returned wrapper owns this namespace. It is physically destroyed
 // after status/participant reads, without pretending Query adds a ref/lock.
 bool frame_wrapper_busy=false;
 template<class Target> bool with_frame_pregame(Target&& target){
  current();if(frame_wrapper_busy){failed=true;throw std::runtime_error("SP frame Pregame factory reentered");}
  need(returned&&owner,"SP frame Pregame actual singleton absent");
#ifdef _WIN32
  need(heap!=INVALID_HANDLE_VALUE&&heap!=nullptr,"SP frame Pregame actual singleton heap absent");
#endif
  frame_wrapper_busy=true;Wrapper* actual=nullptr;
  const auto cleanup=[&]()noexcept{
   if(!actual)return;
   actual->~Wrapper();
#ifdef _WIN32
   if(os_calls==UINT64_MAX)std::terminate();++os_calls;(void)HeapFree(heap,0,actual);
#endif
   actual=nullptr;
  };
  try{
#ifdef _WIN32
   increment(os_calls);void* allocation=HeapAlloc(heap,0,sizeof(Wrapper));
   need(allocation!=nullptr,"SP frame GetPreGame allocation failed");
   actual=new(allocation) Wrapper{this,owner};current();
   need(actual->context==this&&actual->actual_namespace==owner,"SP frame returned Pregame namespace changed");
   const bool result=std::forward<Target>(target)(*actual->actual_namespace);current();
   increment(os_calls);actual->~Wrapper();(void)HeapFree(heap,0,actual);actual=nullptr;
   frame_wrapper_busy=false;current();return result;
#else
   throw std::runtime_error("SP frame Pregame factory requires actual Windows singleton heap");
#endif
  }catch(...){failed=true;cleanup();frame_wrapper_busy=false;throw;}
 }
