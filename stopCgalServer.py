from data_handler.data_queries import DataQueriesInterface

if __name__ == '__main__':
    print("stopping cgal server")
    dqi = DataQueriesInterface()
    print(dqi.ShutdownCgalServer())
    print("stopped cgal server")