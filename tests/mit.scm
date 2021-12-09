; copy examples from MIT scheme documentation and add related ones.

; Conditionals
; https://www.gnu.org/software/mit-scheme/documentation/stable/mit-scheme-ref/Conditionals.html

(assert (and (= 2 2) (> 2 1)))
(assert (and))
 

; https://groups.csail.mit.edu/mac/ftpdir/scheme-7.4/doc-html/scheme_13.html


; Universl Time https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Universal-Time.html
(assert (integer? (get-universal-time)))

; https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Procedure-Operations.html#Procedure-Operations
(assert (procedure? (lambda (x) x)))
(assert (compound-procedure? (lambda (x) x)))
(assert (not (compiled-procedure? (lambda (x) x))))
(assert (not (procedure? 3)))
(assert (= 18 (apply + (list 3 4 5 6))))
(assert (compiled-procedure? eval))





