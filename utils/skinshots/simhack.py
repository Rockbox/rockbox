#!/usr/bin/env python3
"""Patch a checked-out Rockbox tree so its simulator takes screenshots on its own.

TEST ONLY - never commit the result. Applied by skinshots.py to its private
worktree after each checkout. Every edit asserts its anchor, so a tree the
hack doesn't fit fails loudly instead of half patched.

  RB_AUTODUMP=N  N seconds (default 4) after boot dump the main (and
                 remote) LCD and quit
  RB_AUTOPLAY=1  "start in screen: wps" plays /test.m3u, even with nothing
                 to resume
  recording      the peak meters show fixed levels instead of silence
"""
import sys
import os

def edit(tree, path, old, new, prepend=None):
    p = os.path.join(tree, path)
    s = open(p).read()
    if s.count(old) != 1:
        sys.exit(f"simhack: anchor not found once in {path}:\n{old}")
    s = s.replace(old, new)
    if prepend:
        s = prepend + s
    open(p, "w").write(s)

def main(tree):
    # 1 s ticks instead of 5 s; RB_AUTODUMP=<seconds> (default 4) after boot
    # dumps the main (and remote) LCD and quits
    edit(tree, "uisimulator/common/sim_tasks.c",
'''        queue_wait_w_tmo(&sim_queue, &ev, 5*HZ);
''',
'''        queue_wait_w_tmo(&sim_queue, &ev,
                         getenv("RB_AUTODUMP") ? HZ : 5*HZ); /* TEST ONLY */
''', prepend="#include <stdlib.h>\n")
    edit(tree, "uisimulator/common/sim_tasks.c",
'''            case SYS_TIMEOUT:
                call_storage_idle_notifys(false);
                break;
''',
'''            case SYS_TIMEOUT:
                call_storage_idle_notifys(false);
                { /* TEST ONLY: skinshots */
                    static int n;
                    const char *s = getenv("RB_AUTODUMP");
                    if (s && ++n >= (atoi(s) > 1 ? atoi(s) : 4))
                    {
                        screen_dump();
#ifdef HAVE_REMOTE_LCD
                        remote_screen_dump();
#endif
                        exit(0);
                    }
                }
                break;
''')

    edit(tree, "firmware/target/hosted/rtc.c",
'''    time_t now = time(NULL);
    *tm = *localtime(&now);
''',
'''    time_t now = time(NULL);
#ifdef SIMULATOR /* TEST ONLY: skinshots, clock frozen, 2020-01-01 12:00 UTC */
    if (getenv("RB_AUTODUMP"))
        now = 1577880000;
#endif
    *tm = *localtime(&now);
''', prepend="#include <stdlib.h>\n")

    edit(tree, "apps/root_menu.c",
'''    push_current_activity(ACTIVITY_WPS);
''',
'''    push_current_activity(ACTIVITY_WPS);
#ifdef SIMULATOR /* TEST ONLY: skinshots */
    if (!audstatus && getenv("RB_AUTOPLAY"))
    {
        playlist_create("/", "test.m3u");
        playlist_start(0, 0, 0);
        sleep(HZ);
        audstatus = audio_status();
    }
#endif
''')
    edit(tree, "apps/root_menu.c",
'''        (new_screen == GO_TO_WPS && global_status.resume_index == -1))''',
'''        (new_screen == GO_TO_WPS && global_status.resume_index == -1
#ifdef SIMULATOR /* TEST ONLY: skinshots */
         && !getenv("RB_AUTOPLAY")
#endif
        ))''')

    edit(tree, "apps/recorder/peakmeter.c",
'''#ifdef HAVE_RECORDING
    else
        pcm_calculate_rec_peaks(&pm_cur_left, &pm_cur_right);
#endif
''',
'''#ifdef HAVE_RECORDING
    else
    {
        pcm_calculate_rec_peaks(&pm_cur_left, &pm_cur_right);
#ifdef SIMULATOR /* TEST ONLY: skinshots, fixed levels */
        pm_cur_left = MAX_PEAK / 2;
        pm_cur_right = MAX_PEAK / 4;
#endif
    }
#endif
''')

if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("usage: simhack.py <tree>")
    main(sys.argv[1])
