;;;; Emacs Lisp help for writing rockbox code. ;;;;
;;;; $Id$

;;; The rockbox hacker's C conventions.

;;; After loading this file and added the mode-hook you can in C
;;; files, put something like this to use the rockbox style
;;; automatically:
;;
;;   /* -----------------------------------------------------------------
;;    * local variables:
;;    * eval: (c-set-style "rockbox")
;;    * end:
;;    */
;;

(defconst rockbox-c-style
  '((c-basic-offset . 4)
    (c-comment-only-line-offset . 0)
    (c-hanging-braces-alist     . ((substatement-open before after)))
    (c-offsets-alist . ((topmost-intro        . 0)
			(topmost-intro-cont   . 0)
			(substatement         . +)
			(substatement-open    . 0)
			(statement-case-intro . +)
			(statement-case-open  . 0)
			(case-label           . +)
			))
    )
  "Rockbox C Programming Style")

;; Customizations for all of c-mode, c++-mode, and objc-mode
(defun rockbox-c-mode-common-hook ()
  "Rockbox C mode hook"
  ;; add rockbox style and set it for the current buffer
  (c-add-style "rockbox" rockbox-c-style t)
  (setq tab-width 8
	indent-tabs-mode nil		; Use spaces. Not tabs.
	comment-column 40
	c-font-lock-extra-types (append '("bool"))
	)
  (define-key c-mode-base-map "\C-m" 'newline-and-indent)
  (define-key c-mode-base-map "\M-q" 'c-fill-paragraph)
  (setq c-recognize-knr-p nil)
  )

;; Set this is in your .emacs if you want to use the c-mode-hook as
;; defined here right out of the box.
; (add-hook 'c-mode-common-hook 'rockbox-c-mode-common-hook)
