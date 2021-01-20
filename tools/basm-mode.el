;;; basm-mode.el --- Major Mode for editing BASM Assembly Code -*- lexical-binding: t -*-

;; Copyright (C) 2021 Alexey Kutepov <reximkut@gmail.com>

;; Author: Alexey Kutepov <reximkut@gmail.com>
;; URL: http://github.com/tsoding/bm

;; Permission is hereby granted, free of charge, to any person
;; obtaining a copy of this software and associated documentation
;; files (the "Software"), to deal in the Software without
;; restriction, including without limitation the rights to use, copy,
;; modify, merge, publish, distribute, sublicense, and/or sell copies
;; of the Software, and to permit persons to whom the Software is
;; furnished to do so, subject to the following conditions:

;; The above copyright notice and this permission notice shall be
;; included in all copies or substantial portions of the Software.

;; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
;; EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
;; MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
;; NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
;; BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
;; ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
;; CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
;; SOFTWARE.

;;; Commentary:
;;
;; Major Mode for editing BASM Assembly Code. The language for a
;; simple Virtual Machine.

(defvar basm-mode-syntax-table nil "Syntax table for `basm-mode'.")

(setq basm-mode-syntax-table
      (let ((syn-table (make-syntax-table)))
        ;; assembly style comment: “; …”
        (modify-syntax-entry ?\; "<" syn-table)
        (modify-syntax-entry ?\n ">" syn-table)
        syn-table))

(defun basm--interleave (xs y)
  (let ((result nil))
    (reverse
     (cdr
      (dolist (x xs result)
        (push x result)
        (push y result))))))

(let* ((keywords
        '("nop" "push" "drop" "dup" "plusi" "minusi" "multi"
          "divi" "modi" "plusf" "minusf" "multf" "divf" "jmp_if"
          "jmp" "halt" "swap" "not" "eqi" "gei" "gti" "lei"
          "lti" "nei" "eqf" "gef" "gtf" "lef" "ltf" "nef" "ret"
          "call" "native" "andb" "orb" "xor" "shr" "shl" "notb"
          "read8" "read16" "read32" "read64" "write8" "write16"
          "write32" "write64" "i2f" "u2f" "f2i" "f2u"))
       (keywords-regexp
        (concat "\\_<\\("
                (string-join
                 (basm--interleave keywords "\\|"))
                "\\)\\_>")))
  (setq basm-highlights
        `(("%[[:word:]_]+" . font-lock-preprocessor-face)
          ("[[:word:]_]+\\:" . font-lock-constant-face)
          (,keywords-regexp . font-lock-keyword-face))))

;;;###autoload
(define-derived-mode basm-mode fundamental-mode "basm"
  "Major Mode for editing BASM Assembly Code."
  (setq font-lock-defaults '(basm-highlights))
  (set-syntax-table basm-mode-syntax-table))

;;;###autoload
(add-to-list 'auto-mode-alist '("\\.\\(b\\|h\\)asm\\'" . basm-mode))

(provide 'basm-mode)

;;; TODO(#123): basm-mode does not highlight character literals

;;; basm-mode.el ends here
