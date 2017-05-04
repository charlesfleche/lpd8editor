create table pads(programId integer NOT NULL,
                  controlId integer NOT NULL,
                  note integer NOT NULL,
                  pc integer NOT NULL,
                  cc integer NOT NULL,
                  toggle integer NOT NULL,
                  PRIMARY KEY (programId, controlId),
                  FOREIGN KEY (programId) REFERENCES programs (programId));
