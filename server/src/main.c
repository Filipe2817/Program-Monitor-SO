#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../../common/include/utils.h"
#include "../../common/include/request.h"
#include "../../common/include/hashtable.h"

int createNewFifo(const char *fifo_name)
{
    struct stat stats;

    if (stat(fifo_name, &stats) < 0)
    { // Stat failed
        // If (errno == ENOENT), the file or directory does not exist
        // If (errno != ENOENT), an error occurred that is unrelated to the file or directory not existing (Stat failed)
        THROW_ERROR_IF_FAILED_WITH_RETURN(errno != ENOENT, "Stat failed\n");
    }
    else
    {
        // The fifo already exists so we need to delete it
        THROW_ERROR_IF_FAILED_WITH_RETURN(unlink(fifo_name) < 0, "Unlink failed\n");
    }

    // Create new fifo
    THROW_ERROR_IF_FAILED_WITH_RETURN(mkfifo(fifo_name, 0666) < 0, "Fifo creation failed\n");

    return 0;
}

Request bytes_to_request(char *);

int main()
{
    char buf[1024];

    int fifo_return = createNewFifo(FIFO_NAME);
    THROW_ERROR_IF_FAILED_WITH_RETURN(fifo_return == -1, "Error creating FIFO\n");

    file_desc fifo = open(FIFO_NAME, O_RDWR);
    THROW_ERROR_IF_FAILED_WITH_RETURN(fifo == -1, "Error opening FIFO\n");

    // file_desc log = open("log.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    // THROW_ERROR_IF_FAILED_WITH_RETURN(log == -1, "Error opening log file\n");

    Hashtable *hashtable = create_hashtable();

    while (1)
    {
        ssize_t bytes_read, written_bytes;
        while ((bytes_read = read(fifo, buf, sizeof(buf) - 1)) > 0)
        {
            THROW_ERROR_IF_FAILED_WITH_RETURN(bytes_read == -1, "Error reading from FIFO\n");
            buf[bytes_read] = '\0';

            printf("Received: %s\n", buf);

            // written_bytes = write(log, buf, bytes_read);
            // THROW_ERROR_IF_FAILED_WITH_RETURN(written_bytes == -1, "Error writing to log file\n");

            Request request = bytes_to_request(buf);

            switch (request.type)
            {
            case REQUEST_EXECUTE_START:
                notify_sender(buf, request.response_fifo_name);
                insert(hashtable, request.pid, request);
                break;
            case REQUEST_EXECUTE_END:
                Request req = get_resquest(hashtable, request.pid);
                
                break;

            case REQUEST_STATUS:
                break;

            default:
                break;
            }
        }
    }

    // close(log);
    close(fifo);
    unlink(FIFO_NAME);
    return 0;
}
