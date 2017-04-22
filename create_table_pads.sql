create table pads(presetId integer,
                  programId integer NOT NULL,
                  controlId integer NOT NULL,
                  note integer NOT NULL,
                  pc integer NOT NULL,
                  cc integer NOT NULL,
                  toggle integer NOT NULL,
                  PRIMARY KEY (presetId, programId, controlId),
                  FOREIGN KEY (presetId) REFERENCES presets (presetId));
