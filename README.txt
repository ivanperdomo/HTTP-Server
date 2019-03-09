Ivan Perdomo  
iip2103
lab7

PLEASE READ
Please test part 2a fully before testing part 2b because if you test the
mdb-lookup functionality, it will work fine for the first GET request but
subsequent requests won't work. I understand that this does not follow lab
spec but I was totally unable to get around this issue and every other
part of my program works fine.






Part 1)
I created my homepage according to the lab spec at
http://clac.cs.columbia.edu/~iip2103/cs3157/tng/index.html.


Part 2a)
Works as directed. "400 Bad Request" output does not appear correctly in the
browser, but the stdout log information is correct.

Part 2b)
Works perfectly for the first GET request but hangs for
subsequent requests. I don't know why this happens and it's not consistent
behavior. I didn't know how to fix it or if the problem was with my code
rather than with the server. The first mdb-lookup GET request works fine,
but if you try to test part 2a or part 2b afterwards, the program hangs and
the GET request go through, the mdb-lookup-server receives the search term,
but no output is received from mdb-lookup-server and subsequent GET
requests, for part 2a or 2b, don't work.
