import java.lang.reflect.Array;
import java.util.*;
import java.util.function.BiPredicate;
import java.util.function.Function;
import java.util.function.Predicate;
import java.util.stream.Collectors;
import java.util.stream.Stream;

class AStarPathingStrategy
        implements PathingStrategy {

    public static class Node {
        private final Point point;
        private final Node prior_node;
        private final int g;
        private final double h;
        private final double f;

        public Node(Point point, Node prior_node, int g, double h) {
            this.point = point;
            this.prior_node = prior_node;
            this.g = g;
            this.h = h;
            this.f = g + h;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;
            Node node = (Node) o;
            return point.equals(node.point);
        }
        @Override
        public int hashCode() {
            return Objects.hash(point);
        }
    }

    public double distance(Point p1, Point p2) {
        return Math.sqrt(Math.pow((p2.x - p1.x), 2) + Math.pow((p2.y - p1.y), 2));
    }


    public List<Point> computePath(Point start, Point end,
                                   Predicate<Point> canPassThrough,
                                   BiPredicate<Point, Point> withinReach,
                                   Function<Point, Stream<Point>> potentialNeighbors) {
        List<Point> path = new LinkedList<>();
        // Openlist: have calculated all values, haven't yet been searched.
        PriorityQueue<Node> openList = new PriorityQueue<>((n1, n2) -> {
            if (n1.f < n2.f) {
                return -1;
            } else if (n1.f > n2.f) {
                return 1;
            }
            return 0; //if tie
        }); //we passed in our comparator directly in as a lambda.
        HashMap<Point, Node> closedList = new HashMap<>(); // searched list, we do not want to check these nodes again.
        //use the point as the key to look up a node in the hash map
        Node currentNode;

        // add a start node to the open list and mark it as the current node
        currentNode = new Node(start, null, 0, distance(start, end));
        openList.add(currentNode);

        while (true) {
            assert currentNode != null;
            if (withinReach.test(currentNode.point, end)) break; // one within reach, we have found the path.

            //Analyze all valid adjacent nodes that are not obstacles.
            Stream<Point> neighbors = potentialNeighbors.apply(currentNode.point).filter(canPassThrough);

            // calculate all the values for each node, we do not care if it is in the closed list at this point.
            List<Point> neighbor_points = neighbors.collect(Collectors.toList());
            List<Node> neighbor_nodes = new ArrayList<>();
            for (Point p : neighbor_points) {
                neighbor_nodes.add(new Node(p, currentNode, currentNode.g + 1, distance(p, end)));
            }

            //filter by if each node is in the closed list.
            neighbor_nodes.removeIf(closedList::containsValue);
            // we do not need to check if each neighbor is an obstacle because that is handled by canPassThrough

            //check if each node is already on the open list, if yes, we check to see if the g-value is better:

            neighbor_nodes.removeIf(openList::contains); //this just makes sure we don't add duplicates to the open list.

            //add to the open list.
                openList.addAll(neighbor_nodes);

                //move current node to the closed list:
                closedList.put(currentNode.point, currentNode);

                currentNode = openList.peek();
                openList.remove(currentNode); // need to remove from the open list
            }
        // once we get within reach, we trace back through the prior nodes to find the path
        while(currentNode.prior_node != null) {
            path.add(0, currentNode.point); // add to path, front of list is the finish point.
            currentNode = currentNode.prior_node; // work our way back.
        }
        return path;
    }
}

