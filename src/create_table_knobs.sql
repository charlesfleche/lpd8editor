create table knobs(programId integer NOT NULL,
                   controlId integer NOT NULL check (controlId >= 1 and controldId <= 8),
                   cc integer NOT NULL check (cc >= 0 and cc <= 127),
                   low integer NOT NULL check (low >= 0 and low <= 127),
                   high integer NOT NULL check (high >= 0 and high <= 127),
                   PRIMARY KEY (programId, controlId),
                   FOREIGN KEY (programId) REFERENCES programs (programId));
