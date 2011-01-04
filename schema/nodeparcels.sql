CREATE TABLE "nodeparcels" (
  "nodeid" INTEGER  NOT NULL,
  "parcelid" INTEGER NOT NULL,
   UNIQUE ("nodeid") ON CONFLICT IGNORE
);
