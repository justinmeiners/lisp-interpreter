; and or expansion
(let ((a 1))
  (if (and (= a 0) (garbage here))
    (assert 0)
    'pass)

  (if (or (= a 1) (garbage here))
    'pass
    (assert 0)))

