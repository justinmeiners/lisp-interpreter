; https://groups.csail.mit.edu/mac/ftpdir/scheme-7.4/doc-html/scheme_7.html

; TODO: add characters to reader
(assert (string=? (make-string 10 #\x) "xxxxxxxxxx"))

(assert (string? "Hi"))
(assert (not (string? 'Hi)))

(assert (= (string-length "") 0))
(assert (= (string-length "The length") 10))
(assert (string=? "PIE" "PIE"))
(assert (not (string=? "PIE" "pie")))

(assert (string=? (list->string (string->list "hello")) "hello"))

; https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Symbols.html
(assert (symbol? 'foo))
(assert (symbol? (car '(a b))))
(assert (not (symbol? "bar")))

(assert (string=? "FLYING-FISH" (symbol->string 'flying-fish)))
 
