;; $Id$ -*- emacs-lisp -*-

;; Here's a sample .emacs file that might help you along the way.
;; Just copy this region and paste it into your .emacs file.  You
;; might not want to use this if you already use c-mode-hooks for
;; other styles.

(load-file "<YOUR-PATH-TO-ROCKBOX>/tools/rockbox-style.el")
(add-hook 'c-mode-common-hook 'rockbox-c-mode-common-hook)

