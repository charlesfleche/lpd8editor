create table knobs(programId integer NOT NULL,
                   controlId integer NOT NULL,
                   cc integer NOT NULL,
                   low integer NOT NULL,
                   high integer NOT NULL,
                   PRIMARY KEY (programId, controlId),
                   FOREIGN KEY (programId) REFERENCES programs (programId));
