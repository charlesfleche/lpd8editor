create table pads(programId integer NOT NULL,
                  controlId integer NOT NULL check (controlId >= 1 and controlId <= 8),
                  note integer NOT NULL check (note >= 0 and note <= 127),
                  pc integer NOT NULL check (pc >= 0 and pc <= 127),
                  cc integer NOT NULL check (cc >= 0 and cc <= 127),
                  toggle integer NOT NULL check (toggle >= 0 and toggle <= 1),
                  PRIMARY KEY (programId, controlId),
                  FOREIGN KEY (programId) REFERENCES programs (programId));
