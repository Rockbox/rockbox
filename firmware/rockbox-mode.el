;;;; Emacs Lisp help for writing rockbox code. ;;;;
;;;; $Id$

;;; In C files, put something like this to load this file automatically:
;;
;;   /* -----------------------------------------------------------------
;;    * local variables:
;;    * eval: (load-file "rockbox-mode.el")
;;    * end:
;;    */
;;
;; (note: make sure to get the path right in the argument to load-file).


;;; The rockbox hacker's C conventions

;;; we use intent-level 4
(setq c-basic-offset 4)
;;; never ever use tabs to indent!
(setq indent-tabs-mode nil)
;;; enable font coloring of type 'bool'
(setq c-font-lock-extra-types (append '("bool") ))
;;; I like this, stolen from Subversion! ;-)
(setq angry-mob-with-torches-and-pitchforks t)
