(:
   Return the name of the person with ID `person0'
   registered in North America.
:)
for $p in fn:doc("auction")//person[@id="person0"] let $n := $p/name/text()
return $n
