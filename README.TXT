Stream-in-stream encapsulation/decapsulation
--------------------------------------------

This is a set of two simple commands which allow some very basic
functionality: concatenation of multiple streams and - on the
other side - splitting into the original streams.


Syntax:

  streamencapsule [name]
  streamdecapsule command [args...]

The streamencapsule command takes an optional argument, which
will be stored into the output stream as metadata
(designated "stream name", but you can put anything there).
It's maximum size is about 65520 bytes and it can't contain
0x00 bytes (implementation limit, not a design limit).

The streamdecapsule program reads one encapsulated stream
from stdin. It forks a child process which gets passed the content
of that stream on its stdin. It won't read more than one stream,
so it can be called again to read the next encapsulated stream
from stdin. The child process can read the "stream name" metadata
from its environment. It is stored into the environment variable
STREAMNAME.

streamencapsule will return 0 on successful operation.

streamdecapsule will return a value different from 0 when itself
stumbles over an error in its operation *or* the forked process
did return a value different from 0.


Example:

It works like this:

(
  echo A | ./streamencapsule "Stream 1";
  echo B | ./streamencapsule "Stream 2"
) | while ./streamdecapsule sh -c "
  echo \$STREAMNAME:; cat
"; do true; done


Complex example:

Of course, this is just for example purposes. In practice, one
would use it for other scenarios.

It was written to send multiple compressed filesystem streams
over a single SSH connection and on the other side, split that
into many files. That would look like this:

ssh backupuser@backuphost \
  sh -c "
    ls -1 /mnt/snapshots/ | while read s; do
      btrfs send /mnt/snapshots/\$s | xz -1 | streamencapsule \$s
    done" \
| while ./streamdecapsule sh -c "
    echo \"reading $STREAMNAME\"
    cat > "\$STREAMNAME.xz"
"; do true; done

That's a bit complicated, admittedly. What it does is this:

* it connects via SSH to backuphost, and over there it will
  execute via shell:
  * output files/directories in /mnt/snapshot
  * read them in a loop into variable $s
  * for each $s, do a btrfs send, compress it with xz,
    and encapsulate it, giving the stream the name $s
* the result is piped out of the SSH session to a loop running
  streamdecapsule locally, which for each run will read
  a single stream and pass it to a child command
  * this child command is a shell session that
    prints some informal output ("reading ..."), and
    pipes the stream content to a file ....xz

