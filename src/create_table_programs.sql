create table programs(programId integer primary key autoincrement,
                      name varchar not null default 'default',
                      channel integer not null default 9 check (channel >= 0 AND channel <= 15));
