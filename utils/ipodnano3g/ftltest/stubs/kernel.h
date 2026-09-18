struct mutex { int locked; };
static inline void mutex_init(struct mutex *m) { m->locked = 0; }
static inline void mutex_lock(struct mutex *m) { m->locked = 1; }
static inline void mutex_unlock(struct mutex *m) { m->locked = 0; }
