#ifndef DISTRIBUTED_H
#define DISTRIBUTED_H

int run_coordinator(const char *port,
                    const char *start_hex,
                    const char *end_hex,
                    unsigned shard_bits);

int run_worker(const char *host_port,
               const char *default_port,
               char **out_start,
               char **out_end);

#endif
