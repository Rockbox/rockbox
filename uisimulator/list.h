#ifndef LIST_H
#define LIST_H


class ListWalk {
 public:
  ListWalk(class List *list);
  void restart();

  void* getNext();

 private:
  int index;
  class List *list;
};

class List {
  friend ListWalk;
 public:
  List();

  void* getNext(void *); // next after this

  int getSize();

  bool add(void *);
  void *remove(void *);

 private:
  void **data;
  int datasize;

  int size;

};


#endif // LIST_H
