CREATE TABLE "waynodes" (
  "wayid" INTEGER NOT NULL,
  "nodeid" TEXT NOT NULL,
   UNIQUE ("wayid","nodeid") ON CONFLICT IGNORE
);
